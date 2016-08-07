// IE Web Notifications
// Copyright (C) 2015-2016, Sergei Zabolotskikh. All rights reserved.
//
// This file is a part of IE Web Notifications project.
// The use and distribution terms for this software are covered by the
// BSD 3-Clause License (https://opensource.org/licenses/BSD-3-Clause)
// which can be found in the file LICENSE at the root of this distribution.
// By using this software in any fashion, you are agreeing to be bound by
// the terms of this license. You must not remove this notice, or
// any other, from this software.

#include "stdafx.h"
#include "D2DNotificationWindow.h"

#include <cstdint>
#include <Winuser.h>
#include <cassert>
#include <sergz.utils/utils.h>
#include <Wincrypt.h>

using namespace ukot::ie_web_notifications;

// uses defined `hr` of HRESULT type.
#define RETURN_ON_FAIL(expr) do {hr = (expr); if (FAILED(hr)) return hr;} while(false);
#define RETURN_VOID_ON_FAIL(expr) do {hr = (expr); assert(SUCCEEDED(hr) && #expr); if (FAILED(hr)) return;} while(false);

namespace {
  const uint32_t kWindowWidth = 350;
  const uint32_t kWindowHeight = 70;
  const uint32_t kWindowPaddingRight = 20;
  const uint32_t kWindowPaddingBottom = 40;
  const uint32_t kTitleHeight = 30;
  const uint32_t kCloseButtonWidth = 20;
  const uint32_t kDesiredIconSize = 40;
}

D2DNotificationWindow::D2DNotificationWindow(const D2DDeviceIndependentResources& d2d, const NotificationBody& notificationBody)
  : m_d2d(d2d)
  , m_notificationBody(notificationBody)
{
  reloadIcon();
}

D2DNotificationWindow::~D2DNotificationWindow(){
}

LRESULT D2DNotificationWindow::OnCreate(const CREATESTRUCT* /*createStruct*/) {
  {
    // Hack: Actually `MonitorFromWindow` and `GetDpiForMonitor` should be used here.
    WTL::CDC hdc = GetDC();
    m_dpi = hdc.GetDeviceCaps(LOGPIXELSX);
  }
  SetMsgHandled(false);
  return 0;
}

void D2DNotificationWindow::OnResize(WPARAM /*wParam*/, const WTL::CSize& size) {
  if (m_renderTarget && FAILED(m_renderTarget->Resize(D2D1::SizeU(size.cx, size.cy)))){
    discardDeviceResources();
  }
  InvalidateRect(nullptr); // otherwise the window is not updated
}

void D2DNotificationWindow::OnPaint(HDC) {
  HRESULT hr = E_FAIL;
  for (int attempt = 0;
       FAILED(hr) /*when hr == D2DERR_RECREATE_TARGET*/ && attempt < 2;
       ++attempt)
  {
    hr = createDeviceResources();
    if (FAILED(hr)) {
      SetMsgHandled(false);
      return;
    }
    m_renderTarget->BeginDraw();
    m_renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
    m_renderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

    D2D1_SIZE_F renderTargetSize = m_renderTarget->GetSize();

    // Draw two rectangles.
    auto iconAreaWidth = kWindowHeight;
    {
      D2D1_RECT_F iconContainer = D2D1::RectF(0, 0, iconAreaWidth, renderTargetSize.height);
      m_renderTarget->FillRectangle(&iconContainer, m_greyBrush);
      // hack
      if (!!m_wicBitmap && !m_d2Bitmap) {
        hr = m_renderTarget->CreateBitmapFromWicBitmap(m_wicBitmap, &m_d2Bitmap);
      }
      if (!!m_d2Bitmap) {
        D2D1_SIZE_F bitmapSize = m_d2Bitmap->GetSize();
        if (bitmapSize.height <= bitmapSize.width) {
          iconContainer.left = (iconAreaWidth - kDesiredIconSize) / 2.f;
          iconContainer.right = (iconAreaWidth + kDesiredIconSize) / 2.f;
          float scale = kDesiredIconSize / bitmapSize.width;
          float desiredHeight = bitmapSize.height * scale;
          iconContainer.top = (iconContainer.bottom - desiredHeight) / 2.f;
          iconContainer.bottom = (iconContainer.bottom + desiredHeight) / 2.f;
        } else {
          iconContainer.top = (iconContainer.bottom - kDesiredIconSize) / 2.f;
          iconContainer.bottom = (iconContainer.bottom + kDesiredIconSize) / 2.f;
          float scale = kDesiredIconSize / bitmapSize.height;
          float desiredWidth = bitmapSize.width * scale;
          iconContainer.left = (iconAreaWidth - desiredWidth) / 2.f;
          iconContainer.right = (iconAreaWidth + desiredWidth) / 2.f;
        }
        m_renderTarget->DrawBitmap(m_d2Bitmap, iconContainer);
      }
    }
    {
      uint32_t titleMargingTop = 10;
      uint32_t titleMargingLeft = 10;
      D2D1_RECT_F titleRect{};
      titleRect.left = iconAreaWidth + titleMargingLeft;
      titleRect.top = titleMargingTop;
      titleRect.right = renderTargetSize.width - kCloseButtonWidth;
      titleRect.bottom = kTitleHeight;
      m_renderTarget->DrawTextW(m_notificationBody.title.c_str(), m_notificationBody.title.length(),
        m_d2d.titleTextFormat, titleRect, m_blackBrush, D2D1_DRAW_TEXT_OPTIONS::D2D1_DRAW_TEXT_OPTIONS_CLIP);
    }
    {
      uint32_t bodyMargingTop = 0;
      uint32_t bodyMargingLeft = 5;
      D2D1_RECT_F bodyRect{};
      bodyRect.left = iconAreaWidth + bodyMargingLeft;
      bodyRect.top = kTitleHeight + bodyMargingTop;
      bodyRect.right = renderTargetSize.width;
      bodyRect.bottom = renderTargetSize.height;
      m_renderTarget->DrawTextW(m_notificationBody.body.c_str(), m_notificationBody.body.length(),
        m_d2d.bodyTextFormat, bodyRect, m_blackBrush, D2D1_DRAW_TEXT_OPTIONS::D2D1_DRAW_TEXT_OPTIONS_CLIP);
    }
    {// close button
      uint32_t buttonMargin = 5;
      // hacky names, sorry
      m_closeButtonRect.left = renderTargetSize.width - kCloseButtonWidth - buttonMargin;
      m_closeButtonRect.right = renderTargetSize.width - buttonMargin;
      m_closeButtonRect.top = 0;
      m_closeButtonRect.bottom = kCloseButtonWidth - buttonMargin;
      m_renderTarget->DrawRectangle(m_closeButtonRect, m_greyBrush);
      D2D1_POINT_2F centerPoint{ m_closeButtonRect.left + (m_closeButtonRect.right - m_closeButtonRect.left) / 2.f,
        m_closeButtonRect.top + (m_closeButtonRect.bottom - m_closeButtonRect.top) / 2.f };
      float halfCrossSize = 4;
      D2D1_POINT_2F topLeftPoint = { centerPoint.x - halfCrossSize, centerPoint.y - halfCrossSize };
      D2D1_POINT_2F topRightPoint{ centerPoint.x + halfCrossSize, centerPoint.y - halfCrossSize };
      D2D1_POINT_2F bottomLeftPoint = { centerPoint.x - halfCrossSize, centerPoint.y + halfCrossSize };
      D2D1_POINT_2F bottomRightPoint{ centerPoint.x + halfCrossSize, centerPoint.y + halfCrossSize };
      if (m_closeButtonIsMouseOver) {
        m_renderTarget->FillRectangle(m_closeButtonRect, m_redBrush);
        m_renderTarget->DrawLine(topLeftPoint, bottomRightPoint, m_whiteBrush);
        m_renderTarget->DrawLine(bottomLeftPoint, topRightPoint, m_whiteBrush);
      } else {
        m_renderTarget->DrawLine(topLeftPoint, bottomRightPoint, m_blackBrush);
        m_renderTarget->DrawLine(bottomLeftPoint, topRightPoint, m_blackBrush);
      }
    }
    hr = m_renderTarget->EndDraw();
    if (D2DERR_RECREATE_TARGET == hr) {
      discardDeviceResources();
    }
  }
  ValidateRect(/*rect*/nullptr);
}

void D2DNotificationWindow::OnMouseMove(WPARAM, const WTL::CPoint& point) {
  bool closeButtonWasMouseOver = m_closeButtonIsMouseOver;
  WTL::CPoint dpiUnawarePoint(point.x * 96 / m_dpi, point.y * 96 / m_dpi);
  m_closeButtonIsMouseOver = m_closeButtonRect.left <= dpiUnawarePoint.x && dpiUnawarePoint.x <= m_closeButtonRect.right
    && m_closeButtonRect.top <= dpiUnawarePoint.y && dpiUnawarePoint.y <= m_closeButtonRect.bottom;
  if (m_closeButtonIsMouseOver != closeButtonWasMouseOver) {
    Invalidate();
  }

  // Simplified version of mouse tracking, without client area and so on.
  // Good implementation can be found in HWNDMessageHandler::TrackMouseEvents
  // in Chromium project.
  if (!m_isTrackingMouseEvents) {
    TRACKMOUSEEVENT tme{ sizeof(tme) };
    tme.dwFlags = TME_LEAVE;
    tme.hwndTrack = m_hWnd;
    tme.dwHoverTime = HOVER_DEFAULT;
    m_isTrackingMouseEvents = TrackMouseEvent(&tme);
  }
}
void D2DNotificationWindow::OnMouseLeave() {
  m_isTrackingMouseEvents = false;
  if (m_closeButtonIsMouseOver) {
    m_closeButtonIsMouseOver = false;
    Invalidate();
  }
}

void D2DNotificationWindow::OnLMouseBtnUp(WPARAM /*wParam*/, const WTL::CPoint& point) {
  WTL::CPoint dpiUnawarePoint(point.x * 96 / m_dpi, point.y * 96 / m_dpi);
  bool isWithinCloseButton = m_closeButtonRect.left <= dpiUnawarePoint.x && dpiUnawarePoint.x <= m_closeButtonRect.right
    && m_closeButtonRect.top <= dpiUnawarePoint.y && dpiUnawarePoint.y <= m_closeButtonRect.bottom;
  onCloseRequest(!isWithinCloseButton);
}

LRESULT D2DNotificationWindow::OnSettings(UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*handled*/) {
  if (!!onSettingsRequest) {
    onSettingsRequest();
  }
  return 0;
}

void D2DNotificationWindow::setNotificationBody(const NotificationBody& notificationBody) {
  m_notificationBody = notificationBody;
  reloadIcon();
  Invalidate();
}

HRESULT D2DNotificationWindow::createDeviceResources() {
  if (!!m_renderTarget) {
    return S_OK;
  }

  RECT rc{};
  if (!GetClientRect(&rc)) {
    return E_FAIL;
  }
  D2D1_SIZE_U size{ rc.right - rc.left, rc.bottom - rc.top };
  HRESULT hr = S_OK;
  RETURN_ON_FAIL(m_d2d.d2dFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
    D2D1::HwndRenderTargetProperties(m_hWnd, size), &m_renderTarget));
  RETURN_ON_FAIL(m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0xF0F0F0),
    &m_greyBrush));
  RETURN_ON_FAIL(m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black),
    &m_blackBrush));
  RETURN_ON_FAIL(m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red),
    &m_redBrush));
  RETURN_ON_FAIL(m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White),
    &m_whiteBrush));
  return hr;
}

void D2DNotificationWindow::discardDeviceResources() {
  m_d2Bitmap.Release();
  m_redBrush.Release();
  m_whiteBrush.Release();
  m_blackBrush.Release();
  m_greyBrush.Release();
  m_renderTarget.Release();
}

void D2DNotificationWindow::reloadIcon() {
  m_wicBitmap.Release();
  m_d2Bitmap.Release();
  m_imageData.clear();
  auto ii = m_notificationBody.iconFilePath.begin();
  std::wstring::const_iterator ii_end = m_notificationBody.iconFilePath.end();
  utils::leftTrim(ii, ii_end);

  if (ii == ii_end) { // empty
    return;
  }
  HRESULT hr = S_OK;
  ATL::CComPtr<IWICBitmapDecoder> wicBitmapDecoder;

  auto ii_begin = ii;
  if (utils::beginsWith_ci(ii_begin, ii_end, L"data:image/")) {
    // load from stream
    ii = ii_begin; // store the beginning of image type
    // find next ';'
    for (; ii_begin != ii_end && *ii_begin != L';'; ++ii_begin);
    std::wstring imageType{ii, ii_begin};
    if (!utils::beginsWith_ci(ii_begin, ii_end, L";base64,") || ii_begin == ii_end) {
      return;
    }
    { // base64 decode
      DWORD requiredSize = 0;
      BOOL rc = CryptStringToBinaryW(&*ii_begin, ii_end - ii_begin, CRYPT_STRING_BASE64, nullptr, &requiredSize, nullptr, nullptr);
      m_imageData.resize(requiredSize, 0);
      rc = CryptStringToBinaryW(&*ii_begin, ii_end - ii_begin, CRYPT_STRING_BASE64, m_imageData.data(), &requiredSize, nullptr, nullptr);
      if (FALSE == rc) {
        return;
      }
    }
    ATL::CComPtr<IWICStream> wicStream;
    HRESULT hr = S_OK;
    RETURN_VOID_ON_FAIL(m_d2d.wiciFactory->CreateStream(&wicStream));
    RETURN_VOID_ON_FAIL(wicStream->InitializeFromMemory(m_imageData.data(), m_imageData.size()));
    ATL::CComQIPtr<IStream> stream = wicStream;
    if (!stream)
      return;
    // This approach seems not enough safe because the user can have installed some 3rd party soft-
    // ware which can be vulnerable to malicious files.
    // It might worth at least limit to a certain prooved popular formats.
    RETURN_VOID_ON_FAIL(m_d2d.wiciFactory->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnDemand, &wicBitmapDecoder));
  } else {
    // load from file
    RETURN_VOID_ON_FAIL(m_d2d.wiciFactory->CreateDecoderFromFilename(m_notificationBody.iconFilePath.c_str(),
      nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &wicBitmapDecoder));
  }

  UINT frameCount = 0;
  RETURN_VOID_ON_FAIL(wicBitmapDecoder->GetFrameCount(&frameCount));
  ATL::CComPtr<IWICBitmapFrameDecode> imageFrame;
  UINT frameSize = 0;
  UINT idealSize = DPIAware(kDesiredIconSize);
  for (uint32_t i = 0; i < frameCount; ++i) {
    ATL::CComPtr<IWICBitmapFrameDecode> frame;
    RETURN_VOID_ON_FAIL(wicBitmapDecoder->GetFrame(i, &frame));
    UINT width = 0;
    UINT height = 0;
    auto maxSize = std::max(width, height); // along any dimension
    frame->GetSize(&width, &height);
    if (!imageFrame) {
      imageFrame = frame;
      frameSize = maxSize;
    } else {
      if (maxSize >= idealSize && maxSize < frameSize) {
        imageFrame = frame;
        frameSize = maxSize;
      }
    }
  }
  if (!imageFrame) {
    return;
  }
  // scaling is done in render method, it looks much better, however using of IWICBitmapScaler
  // results in smoothed (bad quality) image.
  ATL::CComPtr<IWICFormatConverter> convertedSourceBitmap;
  RETURN_VOID_ON_FAIL(m_d2d.wiciFactory->CreateFormatConverter(&convertedSourceBitmap));
  RETURN_VOID_ON_FAIL(convertedSourceBitmap->Initialize(imageFrame, GUID_WICPixelFormat32bppPBGRA,
    WICBitmapDitherType::WICBitmapDitherTypeNone, /*palette*/nullptr, /*alpha threshold*/0.f,
    WICBitmapPaletteType::WICBitmapPaletteTypeCustom));
  // TODO: do conversion only once and then store cached result
  convertedSourceBitmap.QueryInterface(&m_wicBitmap);
}

D2DNotificationBorderWindow::D2DNotificationBorderWindow(const D2DDeviceIndependentResources& d2d, const NotificationBody& notificationBody)
  : m_d2d(d2d), m_content(d2d, notificationBody)
{
  m_content.onCloseRequest = [this](bool clicked) {
    m_clicked = clicked;
    PostMessage(WM_CLOSE);
  };
  m_content.onSettingsRequest = [this]() {
    if (!!m_onSettingsRequest) {
      m_onSettingsRequest();
    }
  };
}

D2DNotificationBorderWindow:: ~D2DNotificationBorderWindow() {
}

LRESULT D2DNotificationBorderWindow::OnCreate(const CREATESTRUCT* createStruct)
{
  auto windowCoords = getWindowCoordinates();
  MoveWindow(windowCoords.x, windowCoords.y, DPIAware(kWindowWidth), DPIAware(kWindowHeight));

  RECT clientRect;
  GetClientRect(&clientRect);
  // make one pixel border
  clientRect.top += 1;
  clientRect.left += 1;
  clientRect.bottom -= 1;
  clientRect.right -= 1;
  m_content.Create(m_hWnd, clientRect, nullptr, WS_CHILD | WS_VISIBLE);
  auto err = GetLastError();
  SetMsgHandled(false);
  return 0;
}

void D2DNotificationBorderWindow::OnResize(WPARAM /*wParam*/, const WTL::CSize& size) {
  if (m_renderTarget && FAILED(m_renderTarget->Resize(D2D1::SizeU(size.cx, size.cy)))){
    discardDeviceResources();
  }
  InvalidateRect(nullptr); // otherwise the window is not updated
}

void D2DNotificationBorderWindow::OnPaint(HDC) {
  HRESULT hr = E_FAIL;
  for (int attempt = 0;
    FAILED(hr) /*when hr == D2DERR_RECREATE_TARGET*/ && attempt < 2;
    ++attempt)
  {
    hr = createDeviceResources();
    if (FAILED(hr)) {
      SetMsgHandled(false);
      return;
    }
    m_renderTarget->BeginDraw();
    m_renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
    m_renderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));
    hr = m_renderTarget->EndDraw();
    if (D2DERR_RECREATE_TARGET == hr) {
      discardDeviceResources();
    }
  }
  ValidateRect(/*rect*/nullptr);
}

HRESULT D2DNotificationBorderWindow::createDeviceResources() {
  if (!!m_renderTarget) {
    return S_OK;
  }

  RECT rc{};
  if (!GetClientRect(&rc)) {
    return E_FAIL;
  }
  D2D1_SIZE_U size{ rc.right - rc.left, rc.bottom - rc.top };
  HRESULT hr = S_OK;
  RETURN_ON_FAIL(m_d2d.d2dFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
    D2D1::HwndRenderTargetProperties(m_hWnd, size), &m_renderTarget));
  return hr;
}

void D2DNotificationBorderWindow::discardDeviceResources() {
  m_renderTarget.Release();
}

void D2DNotificationBorderWindow::setSlot(uint8_t value) {
  auto windowCoords = getWindowCoordinates();
  MoveWindow(windowCoords.x, windowCoords.y - value * (DPIAware(kWindowHeight) + /*space between windows*/DPIAware(20)),
    DPIAware(kWindowWidth), DPIAware(kWindowHeight));
}

void D2DNotificationBorderWindow::OnFinalMessage(HWND) {
  if (!!m_onDestroyedCallback) {
    m_onDestroyedCallback(m_clicked);
  }
}

POINT D2DNotificationBorderWindow::getWindowCoordinates() {
  HMONITOR primaryMonitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTOPRIMARY);
  {
    WTL::CDC hdc = GetDC();
    m_dpi = hdc.GetDeviceCaps(LOGPIXELSX);
  }
  MONITORINFO monitorInfo{};
  monitorInfo.cbSize = sizeof(monitorInfo);
  GetMonitorInfo(primaryMonitor, &monitorInfo);
  int windowX = monitorInfo.rcWork.right - DPIAware(kWindowWidth + kWindowPaddingRight);
  int windowY = monitorInfo.rcWork.bottom - DPIAware(kWindowHeight + kWindowPaddingBottom);
  return POINT{ windowX, windowY };
}