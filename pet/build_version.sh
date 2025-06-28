#!/bin/bash

# �汾�����ű�
# �����Զ����ɰ汾��Ϣ�͹����̼�

set -e

# ��ɫ����
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# ����
PROJECT_NAME="pet-feeder"
BUILD_DIR="build"
VERSION_FILE="main/version_info.h"

# ��ȡGit��Ϣ
GIT_HASH=$(git rev-parse --short HEAD 2>/dev/null || echo "unknown")
GIT_BRANCH=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unknown")
GIT_TAG=$(git describe --tags --exact-match 2>/dev/null || echo "")

# ��ȡ����ʱ��
BUILD_DATE=$(date +"%Y-%m-%d")
BUILD_TIME=$(date +"%H:%M:%S")
BUILD_TIMESTAMP=$(date +%s)

# �汾�Ž���
if [ -n "$GIT_TAG" ]; then
    # ��Git��ǩ�����汾��
    VERSION_MAJOR=$(echo $GIT_TAG | sed 's/v\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\).*/\1/')
    VERSION_MINOR=$(echo $GIT_TAG | sed 's/v\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\).*/\2/')
    VERSION_PATCH=$(echo $GIT_TAG | sed 's/v\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\).*/\3/')
    VERSION_SUFFIX="stable"
else
    # ��version.h�ļ���ȡ�汾��
    if [ -f "main/version.h" ]; then
        VERSION_MAJOR=$(grep "FIRMWARE_VERSION_MAJOR" main/version.h | sed 's/.*FIRMWARE_VERSION_MAJOR[[:space:]]*\([0-9]*\).*/\1/')
        VERSION_MINOR=$(grep "FIRMWARE_VERSION_MINOR" main/version.h | sed 's/.*FIRMWARE_VERSION_MINOR[[:space:]]*\([0-9]*\).*/\1/')
        VERSION_PATCH=$(grep "FIRMWARE_VERSION_PATCH" main/version.h | sed 's/.*FIRMWARE_VERSION_PATCH[[:space:]]*\([0-9]*\).*/\1/')
        VERSION_SUFFIX=$(grep "FIRMWARE_VERSION_SUFFIX" main/version.h | sed 's/.*FIRMWARE_VERSION_SUFFIX[[:space:]]*"\([^"]*\)".*/\1/')
    else
        echo -e "${RED}����: �Ҳ���version.h�ļ�${NC}"
        exit 1
    fi
fi

# �����ţ�ʹ��ʱ�����
BUILD_NUMBER=$BUILD_TIMESTAMP

# ���ɰ汾��Ϣͷ�ļ�
echo -e "${BLUE}���ɰ汾��Ϣ...${NC}"
cat > $VERSION_FILE << EOF
// �Զ����ɵİ汾��Ϣ�ļ�
// ����ʱ��: $BUILD_DATE $BUILD_TIME
// �����ֶ��޸Ĵ��ļ�

#pragma once

// �汾�Ŷ���
#define FIRMWARE_VERSION_MAJOR    $VERSION_MAJOR
#define FIRMWARE_VERSION_MINOR    $VERSION_MINOR
#define FIRMWARE_VERSION_PATCH    $VERSION_PATCH
#define FIRMWARE_VERSION_BUILD    $BUILD_NUMBER
#define FIRMWARE_VERSION_SUFFIX   "$VERSION_SUFFIX"

// ������Ϣ
#define BUILD_DATE                "$BUILD_DATE"
#define BUILD_TIME                "$BUILD_TIME"
#define BUILD_TIMESTAMP           $BUILD_TIMESTAMP
#define GIT_HASH                  "$GIT_HASH"
#define GIT_BRANCH                "$GIT_BRANCH"
#define GIT_TAG                   "$GIT_TAG"

// �汾�ַ���
#define VERSION_STRING            "v$VERSION_MAJOR.$VERSION_MINOR.$VERSION_PATCH-$BUILD_NUMBER-$VERSION_SUFFIX"
EOF

echo -e "${GREEN}�汾��Ϣ������:${NC}"
echo -e "  �汾��: v$VERSION_MAJOR.$VERSION_MINOR.$VERSION_PATCH"
echo -e "  ������: $BUILD_NUMBER"
echo -e "  ��׺: $VERSION_SUFFIX"
echo -e "  Git��ϣ: $GIT_HASH"
echo -e "  Git��֧: $GIT_BRANCH"
if [ -n "$GIT_TAG" ]; then
    echo -e "  Git��ǩ: $GIT_TAG"
fi
echo -e "  ����ʱ��: $BUILD_DATE $BUILD_TIME"

# ��鹹������
BUILD_TYPE=${1:-release}

case $BUILD_TYPE in
    debug)
        echo -e "${YELLOW}��������: Debug${NC}"
        BUILD_FLAGS=""
        ;;
    release)
        echo -e "${YELLOW}��������: Release${NC}"
        BUILD_FLAGS=""
        ;;
    *)
        echo -e "${RED}δ֪�Ĺ�������: $BUILD_TYPE${NC}"
        echo -e "�÷�: $0 [debug|release]"
        exit 1
        ;;
esac

# ������Ŀ¼
if [ "$2" = "clean" ]; then
    echo -e "${BLUE}������Ŀ¼...${NC}"
    rm -rf $BUILD_DIR
fi

# ������Ŀ
echo -e "${BLUE}��ʼ������Ŀ...${NC}"
idf.py build $BUILD_FLAGS

if [ $? -eq 0 ]; then
    echo -e "${GREEN}�����ɹ�!${NC}"
    
    # ���ɹ̼���Ϣ
    FIRMWARE_FILE="$BUILD_DIR/$PROJECT_NAME.bin"
    if [ -f "$FIRMWARE_FILE" ]; then
        FIRMWARE_SIZE=$(stat -c%s "$FIRMWARE_FILE")
        echo -e "${GREEN}�̼���Ϣ:${NC}"
        echo -e "  �ļ�: $FIRMWARE_FILE"
        echo -e "  ��С: $(($FIRMWARE_SIZE / 1024)) KB"
        echo -e "  �汾: v$VERSION_MAJOR.$VERSION_MINOR.$VERSION_PATCH-$BUILD_NUMBER-$VERSION_SUFFIX"
    fi
    
    # ���ɰ汾����
    VERSION_REPORT="$BUILD_DIR/version_report.txt"
    cat > $VERSION_REPORT << EOF
�汾��������
=============

��Ŀ����: $PROJECT_NAME
����ʱ��: $BUILD_DATE $BUILD_TIME
��������: $BUILD_TYPE

�汾��Ϣ:
  ���汾��: $VERSION_MAJOR
  �ΰ汾��: $VERSION_MINOR
  �����汾��: $VERSION_PATCH
  ������: $BUILD_NUMBER
  �汾��׺: $VERSION_SUFFIX
  �����汾��: v$VERSION_MAJOR.$VERSION_MINOR.$VERSION_PATCH-$BUILD_NUMBER-$VERSION_SUFFIX

Git��Ϣ:
  �ύ��ϣ: $GIT_HASH
  ��֧: $GIT_BRANCH
  ��ǩ: $GIT_TAG

������Ϣ:
  ����ʱ���: $BUILD_TIMESTAMP
  �̼��ļ�: $FIRMWARE_FILE
  �̼���С: $(($FIRMWARE_SIZE / 1024)) KB

��������:
  ����ϵͳ: $(uname -s)
  �ܹ�: $(uname -m)
  ESP-IDF�汾: $(idf.py --version 2>/dev/null | head -1 || echo "unknown")
EOF
    
    echo -e "${GREEN}�汾����������: $VERSION_REPORT${NC}"
    
else
    echo -e "${RED}����ʧ��!${NC}"
    exit 1
fi

echo -e "${GREEN}�������!${NC}" 