#!/bin/bash

# 스크립트 실행 시 오류 발생 시 종료
set -e

# 색상 정의
GREEN="\033[1;32m"
YELLOW="\033[1;33m"
RED="\033[1;31m"
CYAN="\033[1;36m"
RESET="\033[0m"

# 시작 메시지
echo -e "${CYAN}=====================================${RESET}"
echo -e "${CYAN}          🚀 빌드 스크립트 시작        ${RESET}"
echo -e "${CYAN}=====================================${RESET}"

# 기존 빌드 디렉터리 제거
if [ -d "build" ]; then
    echo -e "${YELLOW}[1/6] 기존 빌드 디렉터리를 삭제 중...${RESET}"
    rm -rf build
    echo -e "${GREEN}[완료] 기존 빌드 디렉터리 삭제됨.${RESET}"
else
    echo -e "${YELLOW}[1/6] 기존 빌드 디렉터리가 존재하지 않음.${RESET}"
fi

# 빌드 디렉터리 생성
echo -e "${YELLOW}[2/6] 빌드 디렉터리를 생성 중...${RESET}"
mkdir build
cd build
echo -e "${GREEN}[완료] 빌드 디렉터리 생성됨.${RESET}"

# CMake 설정
echo -e "${YELLOW}[3/6] CMake 설정 중...${RESET}"
cmake -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_FLAGS="-isystem /Library/Developer/CommandLineTools/SDKs/MacOSX15.2.sdk/usr/include/c++/v1" ..
echo -e "${GREEN}[완료] CMake 설정 완료.${RESET}"

# 빌드 수행
echo -e "${YELLOW}[4/6] 프로젝트 빌드 중...${RESET}"
make
echo -e "${GREEN}[완료] 프로젝트 빌드 성공.${RESET}"

# 빌드 완료 후 실행
echo -e "${YELLOW}[5/6] 빌드된 프로그램 실행 중...${RESET}"
if [ -f "OpenCVExample" ]; then
    ./OpenCVExample
    echo -e "${GREEN}[완료] 프로그램 실행 종료.${RESET}"
else
    echo -e "${RED}[오류] 빌드된 실행 파일을 찾을 수 없습니다.${RESET}"
    exit 1
fi

# 종료 메시지
echo -e "${CYAN}=====================================${RESET}"
echo -e "${CYAN}          🎉 빌드 스크립트 완료         ${RESET}"
echo -e "${CYAN}=====================================${RESET}"
