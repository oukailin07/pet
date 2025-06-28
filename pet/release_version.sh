#!/bin/bash

# �汾�����ű�
# �����Զ����汾��������

set -e

# ��ɫ����
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# ����
PROJECT_NAME="pet-feeder"
RELEASE_DIR="releases"
CHANGELOG_FILE="CHANGELOG.md"

# ��ʾ������Ϣ
show_help() {
    echo -e "${BLUE}�汾�����ű�${NC}"
    echo -e "�÷�: $0 [ѡ��] <�汾��>"
    echo -e ""
    echo -e "ѡ��:"
    echo -e "  -h, --help     ��ʾ�˰�����Ϣ"
    echo -e "  -p, --patch    ���������汾 (1.0.0 -> 1.0.1)"
    echo -e "  -m, --minor    �����ΰ汾 (1.0.0 -> 1.1.0)"
    echo -e "  -M, --major    �������汾 (1.0.0 -> 2.0.0)"
    echo -e "  -t, --tag      ����Git��ǩ"
    echo -e "  -b, --build    �����̼�"
    echo -e "  -u, --upload   �ϴ���������"
    echo -e ""
    echo -e "ʾ��:"
    echo -e "  $0 -p -t -b        # ���������汾��������ǩ�������̼�"
    echo -e "  $0 -m -t -b -u     # �����ΰ汾��������ǩ�������̼����ϴ�"
    echo -e "  $0 v1.2.3 -t -b    # ����ָ���汾"
}

# ������ǰ�汾��
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

# �����°汾��
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
            echo -e "${RED}����İ汾����: $version_type${NC}"
            exit 1
            ;;
    esac
    
    echo "$major.$minor.$patch"
}

# ���°汾��
update_version() {
    local new_version=$1
    local major=$(echo $new_version | cut -d. -f1)
    local minor=$(echo $new_version | cut -d. -f2)
    local patch=$(echo $new_version | cut -d. -f3)
    
    echo -e "${BLUE}���°汾�ŵ� v$new_version...${NC}"
    
    # ����ԭ�ļ�
    cp main/version.h main/version.h.bak
    
    # ���°汾��
    sed -i "s/FIRMWARE_VERSION_MAJOR[[:space:]]*[0-9]*/FIRMWARE_VERSION_MAJOR    $major/" main/version.h
    sed -i "s/FIRMWARE_VERSION_MINOR[[:space:]]*[0-9]*/FIRMWARE_VERSION_MINOR    $minor/" main/version.h
    sed -i "s/FIRMWARE_VERSION_PATCH[[:space:]]*[0-9]*/FIRMWARE_VERSION_PATCH    $patch/" main/version.h
    
    echo -e "${GREEN}�汾���Ѹ���${NC}"
}

# ����Git��ǩ
create_git_tag() {
    local version=$1
    
    echo -e "${BLUE}����Git��ǩ v$version...${NC}"
    
    # ����Ƿ���δ�ύ�ĸ���
    if [ -n "$(git status --porcelain)" ]; then
        echo -e "${YELLOW}����: ��δ�ύ�ĸ���${NC}"
        git status --short
        read -p "�Ƿ����? (y/N): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            echo -e "${RED}ȡ������${NC}"
            exit 1
        fi
    fi
    
    # �ύ�汾����
    git add main/version.h
    git commit -m "Bump version to v$version"
    
    # ������ǩ
    git tag -a "v$version" -m "Release v$version"
    
    echo -e "${GREEN}Git��ǩ�Ѵ���: v$version${NC}"
}

# �����̼�
build_firmware() {
    local version=$1
    
    echo -e "${BLUE}�����̼� v$version...${NC}"
    
    # ���й����ű�
    ./build_version.sh release
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}�̼������ɹ�${NC}"
    else
        echo -e "${RED}�̼�����ʧ��${NC}"
        exit 1
    fi
}

# ����������
create_release_package() {
    local version=$1
    
    echo -e "${BLUE}����������...${NC}"
    
    # ��������Ŀ¼
    mkdir -p $RELEASE_DIR
    
    # ���ƹ̼��ļ�
    local firmware_file="build/pet.bin"
    local release_file="$RELEASE_DIR/pet-feeder-v$version.bin"
    
    if [ -f "$firmware_file" ]; then
        cp "$firmware_file" "$release_file"
        echo -e "${GREEN}�������Ѵ���: $release_file${NC}"
    else
        echo -e "${RED}����: �Ҳ����̼��ļ� $firmware_file${NC}"
        exit 1
    fi
    
    # ���ɷ�����Ϣ
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
    
    echo -e "${GREEN}������Ϣ������: $release_info${NC}"
}

# ���±����־
update_changelog() {
    local version=$1
    local release_date=$(date +"%Y-%m-%d")
    
    echo -e "${BLUE}���±����־...${NC}"
    
    # ���������־�ļ�����������ڣ�
    if [ ! -f "$CHANGELOG_FILE" ]; then
        cat > "$CHANGELOG_FILE" << EOF
# �����־

���ĵ���¼�˳���ιʳ����Ŀ��������Ҫ�����

��ʽ���� [Keep a Changelog](https://keepachangelog.com/zh-CN/1.0.0/)��
���ұ���Ŀ��ѭ [���廯�汾](https://semver.org/lang/zh-CN/)��

## [δ����]

### ����
### ���
### �޸�
### �Ƴ�

EOF
    fi
    
    # ��ȡGit�ύ��¼
    local last_tag=$(git describe --tags --abbrev=0 2>/dev/null || echo "")
    local commits=""
    
    if [ -n "$last_tag" ]; then
        commits=$(git log --pretty=format:"- %s" ${last_tag}..HEAD)
    else
        commits=$(git log --pretty=format:"- %s" --oneline -10)
    fi
    
    # ���ļ���ͷ�����°汾
    local temp_file=$(mktemp)
    cat > "$temp_file" << EOF
# �����־

���ĵ���¼�˳���ιʳ����Ŀ��������Ҫ�����

��ʽ���� [Keep a Changelog](https://keepachangelog.com/zh-CN/1.0.0/)��
���ұ���Ŀ��ѭ [���廯�汾](https://semver.org/lang/zh-CN/)��

## [δ����]

### ����
### ���
### �޸�
### �Ƴ�

## [v$version] - $release_date

### ����
### ���
### �޸�
### �Ƴ�

### ����ϸ��
- ����ʱ��: $(date)
- Git�ύ: $(git rev-parse --short HEAD)
- Э��汾: 1.0
- Ӳ���汾: 1.0

EOF
    
    # �����������
    tail -n +12 "$CHANGELOG_FILE" >> "$temp_file"
    mv "$temp_file" "$CHANGELOG_FILE"
    
    echo -e "${GREEN}�����־�Ѹ���${NC}"
}

# �ϴ���������
upload_to_server() {
    local version=$1
    
    echo -e "${BLUE}�ϴ���������...${NC}"
    
    # ����Ӧ��ʵ�־�����ϴ��߼�
    # ���磺�ϴ���Web��������OTA��������
    
    local release_file="$RELEASE_DIR/pet-feeder-v$version.bin"
    local release_info="$RELEASE_DIR/release-v$version.json"
    
    if [ -f "$release_file" ] && [ -f "$release_info" ]; then
        echo -e "${YELLOW}ע��: �ϴ�������Ҫ���÷�������Ϣ${NC}"
        echo -e "�����ļ�: $release_file"
        echo -e "������Ϣ: $release_info"
        # TODO: ʵ��ʵ�ʵ��ϴ��߼�
    else
        echo -e "${RED}����: �Ҳ��������ļ�${NC}"
        exit 1
    fi
}

# ������
main() {
    local version_type=""
    local new_version=""
    local create_tag=false
    local build_firmware_flag=false
    local upload_flag=false
    
    # ���������в���
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
                echo -e "${RED}δ֪����: $1${NC}"
                show_help
                exit 1
                ;;
        esac
    done
    
    # ���û��ָ���汾�ţ�������°汾��
    if [ -z "$new_version" ]; then
        if [ -z "$version_type" ]; then
            echo -e "${RED}����: ��ָ���汾���ͻ�汾��${NC}"
            show_help
            exit 1
        fi
        
        local current_version=$(get_current_version)
        new_version=$(calculate_new_version $current_version $version_type)
    fi
    
    echo -e "${GREEN}��ʼ�����汾 v$new_version${NC}"
    
    # ���°汾��
    update_version $new_version
    
    # ���±����־
    update_changelog $new_version
    
    # ����Git��ǩ
    if [ "$create_tag" = true ]; then
        create_git_tag $new_version
    fi
    
    # �����̼�
    if [ "$build_firmware_flag" = true ]; then
        build_firmware $new_version
    fi
    
    # ����������
    create_release_package $new_version
    
    # �ϴ���������
    if [ "$upload_flag" = true ]; then
        upload_to_server $new_version
    fi
    
    echo -e "${GREEN}�汾 v$new_version �������!${NC}"
    echo -e "${BLUE}�����ļ�λ��: $RELEASE_DIR/${NC}"
}

# ����������
main "$@" 