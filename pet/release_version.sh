#!/bin/bash

# 版本发布脚本
# 用于自动化版本发布流程

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 配置
PROJECT_NAME="pet-feeder"
RELEASE_DIR="releases"
CHANGELOG_FILE="CHANGELOG.md"

# 显示帮助信息
show_help() {
    echo -e "${BLUE}版本发布脚本${NC}"
    echo -e "用法: $0 [选项] <版本号>"
    echo -e ""
    echo -e "选项:"
    echo -e "  -h, --help     显示此帮助信息"
    echo -e "  -p, --patch    发布补丁版本 (1.0.0 -> 1.0.1)"
    echo -e "  -m, --minor    发布次版本 (1.0.0 -> 1.1.0)"
    echo -e "  -M, --major    发布主版本 (1.0.0 -> 2.0.0)"
    echo -e "  -t, --tag      创建Git标签"
    echo -e "  -b, --build    构建固件"
    echo -e "  -u, --upload   上传到服务器"
    echo -e ""
    echo -e "示例:"
    echo -e "  $0 -p -t -b        # 发布补丁版本，创建标签，构建固件"
    echo -e "  $0 -m -t -b -u     # 发布次版本，创建标签，构建固件，上传"
    echo -e "  $0 v1.2.3 -t -b    # 发布指定版本"
}

# 解析当前版本号
get_current_version() {
    if [ -f "main/version.h" ]; then
        local major=$(grep "FIRMWARE_VERSION_MAJOR" main/version.h | sed 's/.*FIRMWARE_VERSION_MAJOR[[:space:]]*\([0-9]*\).*/\1/')
        local minor=$(grep "FIRMWARE_VERSION_MINOR" main/version.h | sed 's/.*FIRMWARE_VERSION_MINOR[[:space:]]*\([0-9]*\).*/\1/')
        local patch=$(grep "FIRMWARE_VERSION_PATCH" main/version.h | sed 's/.*FIRMWARE_VERSION_PATCH[[:space:]]*\([0-9]*\).*/\1/')
        echo "$major.$minor.$patch"
    else
        echo "0.0.0"
    fi
}

# 计算新版本号
calculate_new_version() {
    local current_version=$1
    local version_type=$2
    
    local major=$(echo $current_version | cut -d. -f1)
    local minor=$(echo $current_version | cut -d. -f2)
    local patch=$(echo $current_version | cut -d. -f3)
    
    case $version_type in
        major)
            major=$((major + 1))
            minor=0
            patch=0
            ;;
        minor)
            minor=$((minor + 1))
            patch=0
            ;;
        patch)
            patch=$((patch + 1))
            ;;
        *)
            echo -e "${RED}错误的版本类型: $version_type${NC}"
            exit 1
            ;;
    esac
    
    echo "$major.$minor.$patch"
}

# 更新版本号
update_version() {
    local new_version=$1
    local major=$(echo $new_version | cut -d. -f1)
    local minor=$(echo $new_version | cut -d. -f2)
    local patch=$(echo $new_version | cut -d. -f3)
    
    echo -e "${BLUE}更新版本号到 v$new_version...${NC}"
    
    # 备份原文件
    cp main/version.h main/version.h.bak
    
    # 更新版本号
    sed -i "s/FIRMWARE_VERSION_MAJOR[[:space:]]*[0-9]*/FIRMWARE_VERSION_MAJOR    $major/" main/version.h
    sed -i "s/FIRMWARE_VERSION_MINOR[[:space:]]*[0-9]*/FIRMWARE_VERSION_MINOR    $minor/" main/version.h
    sed -i "s/FIRMWARE_VERSION_PATCH[[:space:]]*[0-9]*/FIRMWARE_VERSION_PATCH    $patch/" main/version.h
    
    echo -e "${GREEN}版本号已更新${NC}"
}

# 创建Git标签
create_git_tag() {
    local version=$1
    
    echo -e "${BLUE}创建Git标签 v$version...${NC}"
    
    # 检查是否有未提交的更改
    if [ -n "$(git status --porcelain)" ]; then
        echo -e "${YELLOW}警告: 有未提交的更改${NC}"
        git status --short
        read -p "是否继续? (y/N): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            echo -e "${RED}取消发布${NC}"
            exit 1
        fi
    fi
    
    # 提交版本更新
    git add main/version.h
    git commit -m "Bump version to v$version"
    
    # 创建标签
    git tag -a "v$version" -m "Release v$version"
    
    echo -e "${GREEN}Git标签已创建: v$version${NC}"
}

# 构建固件
build_firmware() {
    local version=$1
    
    echo -e "${BLUE}构建固件 v$version...${NC}"
    
    # 运行构建脚本
    ./build_version.sh release
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}固件构建成功${NC}"
    else
        echo -e "${RED}固件构建失败${NC}"
        exit 1
    fi
}

# 创建发布包
create_release_package() {
    local version=$1
    
    echo -e "${BLUE}创建发布包...${NC}"
    
    # 创建发布目录
    mkdir -p $RELEASE_DIR
    
    # 复制固件文件
    local firmware_file="build/pet.bin"
    local release_file="$RELEASE_DIR/pet-feeder-v$version.bin"
    
    if [ -f "$firmware_file" ]; then
        cp "$firmware_file" "$release_file"
        echo -e "${GREEN}发布包已创建: $release_file${NC}"
    else
        echo -e "${RED}错误: 找不到固件文件 $firmware_file${NC}"
        exit 1
    fi
    
    # 生成发布信息
    local release_info="$RELEASE_DIR/release-v$version.json"
    cat > "$release_info" << EOF
{
    "version": "v$version",
    "release_date": "$(date -u +"%Y-%m-%dT%H:%M:%SZ")",
    "firmware_file": "pet-feeder-v$version.bin",
    "file_size": $(stat -c%s "$release_file"),
    "checksum": "$(sha256sum "$release_file" | cut -d' ' -f1)",
    "git_hash": "$(git rev-parse HEAD)",
    "git_tag": "v$version",
    "build_type": "release",
    "compatibility": {
        "protocol_version": "1.0",
        "hardware_version": "1.0"
    }
}
EOF
    
    echo -e "${GREEN}发布信息已生成: $release_info${NC}"
}

# 更新变更日志
update_changelog() {
    local version=$1
    local release_date=$(date +"%Y-%m-%d")
    
    echo -e "${BLUE}更新变更日志...${NC}"
    
    # 创建变更日志文件（如果不存在）
    if [ ! -f "$CHANGELOG_FILE" ]; then
        cat > "$CHANGELOG_FILE" << EOF
# 变更日志

本文档记录了宠物喂食器项目的所有重要变更。

格式基于 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.0.0/)，
并且本项目遵循 [语义化版本](https://semver.org/lang/zh-CN/)。

## [未发布]

### 新增
### 变更
### 修复
### 移除

EOF
    fi
    
    # 获取Git提交记录
    local last_tag=$(git describe --tags --abbrev=0 2>/dev/null || echo "")
    local commits=""
    
    if [ -n "$last_tag" ]; then
        commits=$(git log --pretty=format:"- %s" ${last_tag}..HEAD)
    else
        commits=$(git log --pretty=format:"- %s" --oneline -10)
    fi
    
    # 在文件开头插入新版本
    local temp_file=$(mktemp)
    cat > "$temp_file" << EOF
# 变更日志

本文档记录了宠物喂食器项目的所有重要变更。

格式基于 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.0.0/)，
并且本项目遵循 [语义化版本](https://semver.org/lang/zh-CN/)。

## [未发布]

### 新增
### 变更
### 修复
### 移除

## [v$version] - $release_date

### 新增
### 变更
### 修复
### 移除

### 技术细节
- 构建时间: $(date)
- Git提交: $(git rev-parse --short HEAD)
- 协议版本: 1.0
- 硬件版本: 1.0

EOF
    
    # 添加其余内容
    tail -n +12 "$CHANGELOG_FILE" >> "$temp_file"
    mv "$temp_file" "$CHANGELOG_FILE"
    
    echo -e "${GREEN}变更日志已更新${NC}"
}

# 上传到服务器
upload_to_server() {
    local version=$1
    
    echo -e "${BLUE}上传到服务器...${NC}"
    
    # 这里应该实现具体的上传逻辑
    # 例如：上传到Web服务器、OTA服务器等
    
    local release_file="$RELEASE_DIR/pet-feeder-v$version.bin"
    local release_info="$RELEASE_DIR/release-v$version.json"
    
    if [ -f "$release_file" ] && [ -f "$release_info" ]; then
        echo -e "${YELLOW}注意: 上传功能需要配置服务器信息${NC}"
        echo -e "发布文件: $release_file"
        echo -e "发布信息: $release_info"
        # TODO: 实现实际的上传逻辑
    else
        echo -e "${RED}错误: 找不到发布文件${NC}"
        exit 1
    fi
}

# 主函数
main() {
    local version_type=""
    local new_version=""
    local create_tag=false
    local build_firmware_flag=false
    local upload_flag=false
    
    # 解析命令行参数
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -p|--patch)
                version_type="patch"
                shift
                ;;
            -m|--minor)
                version_type="minor"
                shift
                ;;
            -M|--major)
                version_type="major"
                shift
                ;;
            -t|--tag)
                create_tag=true
                shift
                ;;
            -b|--build)
                build_firmware_flag=true
                shift
                ;;
            -u|--upload)
                upload_flag=true
                shift
                ;;
            v*)
                new_version=$(echo $1 | sed 's/v//')
                shift
                ;;
            *)
                echo -e "${RED}未知参数: $1${NC}"
                show_help
                exit 1
                ;;
        esac
    done
    
    # 如果没有指定版本号，则计算新版本号
    if [ -z "$new_version" ]; then
        if [ -z "$version_type" ]; then
            echo -e "${RED}错误: 请指定版本类型或版本号${NC}"
            show_help
            exit 1
        fi
        
        local current_version=$(get_current_version)
        new_version=$(calculate_new_version $current_version $version_type)
    fi
    
    echo -e "${GREEN}开始发布版本 v$new_version${NC}"
    
    # 更新版本号
    update_version $new_version
    
    # 更新变更日志
    update_changelog $new_version
    
    # 创建Git标签
    if [ "$create_tag" = true ]; then
        create_git_tag $new_version
    fi
    
    # 构建固件
    if [ "$build_firmware_flag" = true ]; then
        build_firmware $new_version
    fi
    
    # 创建发布包
    create_release_package $new_version
    
    # 上传到服务器
    if [ "$upload_flag" = true ]; then
        upload_to_server $new_version
    fi
    
    echo -e "${GREEN}版本 v$new_version 发布完成!${NC}"
    echo -e "${BLUE}发布文件位置: $RELEASE_DIR/${NC}"
}

# 运行主函数
main "$@" 