#!/bin/bash

# 版本构建脚本
# 用于自动生成版本信息和构建固件

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 配置
PROJECT_NAME="pet-feeder"
BUILD_DIR="build"
VERSION_FILE="main/version_info.h"

# 获取Git信息
GIT_HASH=$(git rev-parse --short HEAD 2>/dev/null || echo "unknown")
GIT_BRANCH=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unknown")
GIT_TAG=$(git describe --tags --exact-match 2>/dev/null || echo "")

# 获取构建时间
BUILD_DATE=$(date +"%Y-%m-%d")
BUILD_TIME=$(date +"%H:%M:%S")
BUILD_TIMESTAMP=$(date +%s)

# 版本号解析
if [ -n "$GIT_TAG" ]; then
    # 从Git标签解析版本号
    VERSION_MAJOR=$(echo $GIT_TAG | sed 's/v\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\).*/\1/')
    VERSION_MINOR=$(echo $GIT_TAG | sed 's/v\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\).*/\2/')
    VERSION_PATCH=$(echo $GIT_TAG | sed 's/v\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\).*/\3/')
    VERSION_SUFFIX="stable"
else
    # 从version.h文件读取版本号
    if [ -f "main/version.h" ]; then
        VERSION_MAJOR=$(grep "FIRMWARE_VERSION_MAJOR" main/version.h | sed 's/.*FIRMWARE_VERSION_MAJOR[[:space:]]*\([0-9]*\).*/\1/')
        VERSION_MINOR=$(grep "FIRMWARE_VERSION_MINOR" main/version.h | sed 's/.*FIRMWARE_VERSION_MINOR[[:space:]]*\([0-9]*\).*/\1/')
        VERSION_PATCH=$(grep "FIRMWARE_VERSION_PATCH" main/version.h | sed 's/.*FIRMWARE_VERSION_PATCH[[:space:]]*\([0-9]*\).*/\1/')
        VERSION_SUFFIX=$(grep "FIRMWARE_VERSION_SUFFIX" main/version.h | sed 's/.*FIRMWARE_VERSION_SUFFIX[[:space:]]*"\([^"]*\)".*/\1/')
    else
        echo -e "${RED}错误: 找不到version.h文件${NC}"
        exit 1
    fi
fi

# 构建号（使用时间戳）
BUILD_NUMBER=$BUILD_TIMESTAMP

# 生成版本信息头文件
echo -e "${BLUE}生成版本信息...${NC}"
cat > $VERSION_FILE << EOF
// 自动生成的版本信息文件
// 生成时间: $BUILD_DATE $BUILD_TIME
// 请勿手动修改此文件

#pragma once

// 版本号定义
#define FIRMWARE_VERSION_MAJOR    $VERSION_MAJOR
#define FIRMWARE_VERSION_MINOR    $VERSION_MINOR
#define FIRMWARE_VERSION_PATCH    $VERSION_PATCH
#define FIRMWARE_VERSION_BUILD    $BUILD_NUMBER
#define FIRMWARE_VERSION_SUFFIX   "$VERSION_SUFFIX"

// 构建信息
#define BUILD_DATE                "$BUILD_DATE"
#define BUILD_TIME                "$BUILD_TIME"
#define BUILD_TIMESTAMP           $BUILD_TIMESTAMP
#define GIT_HASH                  "$GIT_HASH"
#define GIT_BRANCH                "$GIT_BRANCH"
#define GIT_TAG                   "$GIT_TAG"

// 版本字符串
#define VERSION_STRING            "v$VERSION_MAJOR.$VERSION_MINOR.$VERSION_PATCH-$BUILD_NUMBER-$VERSION_SUFFIX"
EOF

echo -e "${GREEN}版本信息已生成:${NC}"
echo -e "  版本号: v$VERSION_MAJOR.$VERSION_MINOR.$VERSION_PATCH"
echo -e "  构建号: $BUILD_NUMBER"
echo -e "  后缀: $VERSION_SUFFIX"
echo -e "  Git哈希: $GIT_HASH"
echo -e "  Git分支: $GIT_BRANCH"
if [ -n "$GIT_TAG" ]; then
    echo -e "  Git标签: $GIT_TAG"
fi
echo -e "  构建时间: $BUILD_DATE $BUILD_TIME"

# 检查构建类型
BUILD_TYPE=${1:-release}

case $BUILD_TYPE in
    debug)
        echo -e "${YELLOW}构建类型: Debug${NC}"
        BUILD_FLAGS=""
        ;;
    release)
        echo -e "${YELLOW}构建类型: Release${NC}"
        BUILD_FLAGS=""
        ;;
    *)
        echo -e "${RED}未知的构建类型: $BUILD_TYPE${NC}"
        echo -e "用法: $0 [debug|release]"
        exit 1
        ;;
esac

# 清理构建目录
if [ "$2" = "clean" ]; then
    echo -e "${BLUE}清理构建目录...${NC}"
    rm -rf $BUILD_DIR
fi

# 构建项目
echo -e "${BLUE}开始构建项目...${NC}"
idf.py build $BUILD_FLAGS

if [ $? -eq 0 ]; then
    echo -e "${GREEN}构建成功!${NC}"
    
    # 生成固件信息
    FIRMWARE_FILE="$BUILD_DIR/$PROJECT_NAME.bin"
    if [ -f "$FIRMWARE_FILE" ]; then
        FIRMWARE_SIZE=$(stat -c%s "$FIRMWARE_FILE")
        echo -e "${GREEN}固件信息:${NC}"
        echo -e "  文件: $FIRMWARE_FILE"
        echo -e "  大小: $(($FIRMWARE_SIZE / 1024)) KB"
        echo -e "  版本: v$VERSION_MAJOR.$VERSION_MINOR.$VERSION_PATCH-$BUILD_NUMBER-$VERSION_SUFFIX"
    fi
    
    # 生成版本报告
    VERSION_REPORT="$BUILD_DIR/version_report.txt"
    cat > $VERSION_REPORT << EOF
版本构建报告
=============

项目名称: $PROJECT_NAME
构建时间: $BUILD_DATE $BUILD_TIME
构建类型: $BUILD_TYPE

版本信息:
  主版本号: $VERSION_MAJOR
  次版本号: $VERSION_MINOR
  补丁版本号: $VERSION_PATCH
  构建号: $BUILD_NUMBER
  版本后缀: $VERSION_SUFFIX
  完整版本号: v$VERSION_MAJOR.$VERSION_MINOR.$VERSION_PATCH-$BUILD_NUMBER-$VERSION_SUFFIX

Git信息:
  提交哈希: $GIT_HASH
  分支: $GIT_BRANCH
  标签: $GIT_TAG

构建信息:
  构建时间戳: $BUILD_TIMESTAMP
  固件文件: $FIRMWARE_FILE
  固件大小: $(($FIRMWARE_SIZE / 1024)) KB

构建环境:
  操作系统: $(uname -s)
  架构: $(uname -m)
  ESP-IDF版本: $(idf.py --version 2>/dev/null | head -1 || echo "unknown")
EOF
    
    echo -e "${GREEN}版本报告已生成: $VERSION_REPORT${NC}"
    
else
    echo -e "${RED}构建失败!${NC}"
    exit 1
fi

echo -e "${GREEN}构建完成!${NC}" 