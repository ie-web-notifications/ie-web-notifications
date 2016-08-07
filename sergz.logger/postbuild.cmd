@echo off

if not exist "%SOLUTIONOUTDIR%logger.properties" (
  mklink /H "%SOLUTIONOUTDIR%logger.properties" logger.%CONFIGURATION%.properties
)