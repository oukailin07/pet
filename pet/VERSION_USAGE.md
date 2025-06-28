# �汾Э��ʹ��˵��

���ĵ���ϸ˵�������ʹ�ó���ιʳ����Ŀ�İ汾�����ܡ�

## 1. ���ٿ�ʼ

### 1.1 ������Ŀ
```bash
# ʹ�ù����ű����Ƽ���
./build_version.sh release

# ��ֱ��ʹ��ESP-IDF
idf.py build
```

### 1.2 �����°汾
```bash
# ���������汾��1.0.0 -> 1.0.1��
./release_version.sh -p -t -b

# �����ΰ汾��1.0.0 -> 1.1.0��
./release_version.sh -m -t -b

# �������汾��1.0.0 -> 2.0.0��
./release_version.sh -M -t -b

# ����ָ���汾
./release_version.sh v1.2.3 -t -b
```

## 2. �汾�Ź淶

### 2.1 �汾�Ÿ�ʽ
```
v<major>.<minor>.<patch>[-<build>][-<suffix>]
```

**ʾ����**
- `v1.0.0` - ��һ���ȶ��汾
- `v1.2.3` - 1.2�汾�ĵ���������
- `v2.0.0-beta1` - 2.0�汾�ĵ�һ�����԰�
- `v1.5.2-20231201` - ���������ڵİ汾

### 2.2 �汾�ŵ�������

#### ���汾�� (major)
- �ش��ܱ��
- �ܹ��ع�
- �����ݵ�API���
- ͨ��Э�鲻���ݱ��

#### �ΰ汾�� (minor)
- �¹������
- ͨ��Э���������չ
- �����Ż�
- Ӳ��֧����չ

#### �����汾�� (patch)
- Bug�޸�
- ��ȫ����
- С���ܸĽ�
- �ĵ�����

## 3. �����ű�ʹ��

### 3.1 build_version.sh
�Զ����ɰ汾��Ϣ�������̼���

```bash
# ���������汾
./build_version.sh release

# �������԰汾
./build_version.sh debug

# ��������
./build_version.sh release clean
```

**���ܣ�**
- �Զ����ɰ汾��Ϣͷ�ļ�
- ��ȡGit�ύ��Ϣ
- �����̼�
- ���ɰ汾����

### 3.2 release_version.sh
�Զ����汾�������̡�

```bash
# �鿴����
./release_version.sh --help

# ���������汾
./release_version.sh -p -t -b

# �����ΰ汾���ϴ�
./release_version.sh -m -t -b -u

# ����ָ���汾
./release_version.sh v1.2.3 -t -b
```

**ѡ��˵����**
- `-p, --patch`: ���������汾
- `-m, --minor`: �����ΰ汾
- `-M, --major`: �������汾
- `-t, --tag`: ����Git��ǩ
- `-b, --build`: �����̼�
- `-u, --upload`: �ϴ���������

## 4. �汾����API

### 4.1 ��ȡ�汾��Ϣ
```c
#include "version.h"

// ��ȡ�̼��汾
const firmware_version_t *fw_version = get_firmware_version();
char version_str[64];
get_version_string(fw_version, version_str, sizeof(version_str));
printf("�̼��汾: %s\n", version_str);

// ��ȡЭ��汾
const protocol_version_t *proto_version = get_protocol_version();
printf("Э��汾: %d.%d\n", proto_version->major, proto_version->minor);

// ��ȡӲ���汾
const hardware_version_t *hw_version = get_hardware_version();
printf("Ӳ���汾: %d.%d\n", hw_version->major, hw_version->minor);
```

### 4.2 �汾�����Լ��
```c
// ���̼�������
firmware_version_t target = {1, 2, 0, 0, "stable"};
bool compatible = check_firmware_compatibility(get_firmware_version(), &target);

// ���Э�������
protocol_version_t server = {1, 1, "Server Protocol"};
bool compatible = check_protocol_compatibility(get_protocol_version(), &server);
```

### 4.3 OTA����
```c
// ��ȡOTA��Ϣ
ota_info_t ota_info;
get_ota_info(&ota_info);

// ����OTA״̬
update_ota_status(OTA_STATUS_DOWNLOADING, 50, 0, "");

// �汾�ع�
rollback_version("v1.0.0");
```

## 5. WebSocketͨ��Э��

### 5.1 �汾���
```json
// �ͻ��˷��Ͱ汾�������
{
    "type": "version_check",
    "device_id": "device_001",
    "firmware_version": "v1.0.0",
    "protocol_version": "1.0",
    "hardware_version": "1.0"
}

// ��������Ӧ
{
    "type": "version_check_result",
    "has_update": true,
    "latest_version": "v1.1.0",
    "download_url": "https://example.com/firmware/v1.1.0.bin",
    "file_size": 1048576,
    "checksum": "sha256:abc123...",
    "force_update": false,
    "description": "�޸���ιʳʱ��ͬ������"
}
```

### 5.2 OTA����
```json
// ��������������ָ��
{
    "type": "ota_update",
    "url": "https://example.com/firmware/v1.1.0.bin",
    "version": "v1.1.0",
    "checksum": "sha256:abc123...",
    "force": false
}

// �ͻ��˱�������״̬
{
    "type": "ota_status",
    "device_id": "device_001",
    "status": "downloading",
    "progress": 75,
    "error_code": 0,
    "error_message": ""
}
```

### 5.3 �汾�ع�
```json
// ���������ͻع�����
{
    "type": "rollback_request",
    "target_version": "v1.0.0",
    "reason": "user_request"
}

// �ͻ�����Ӧ�ع����
{
    "type": "rollback_result",
    "device_id": "device_001",
    "target_version": "v1.0.0",
    "success": true
}
```

## 6. �����ļ�

### 6.1 main/version.h
����汾�ų�����
```c
#define FIRMWARE_VERSION_MAJOR    1
#define FIRMWARE_VERSION_MINOR    0
#define FIRMWARE_VERSION_PATCH    0
#define FIRMWARE_VERSION_BUILD    1
#define FIRMWARE_VERSION_SUFFIX   "stable"
```

### 6.2 main/version_info.h
�Զ����ɵİ汾��Ϣ�ļ����ɹ����ű����ɣ���

## 7. ���ʵ��

### 7.1 �汾��������
1. **�����׶�**
   - ʹ�� `-alpha` �� `-beta` ��׺
   - �������ڲ�����

2. **���Խ׶�**
   - ʹ�� `-rc` ��׺
   - С��Χ����

3. **�����׶�**
   - ʹ�� `-stable` ��׺���޺�׺
   - ��ʽ����

### 7.2 �汾������
- ͬһ���汾���ڱ�����ȫ����
- ��ͬ���汾����Ҫ���⴦��
- Э��汾��ѭ������ԭ��

### 7.3 ��ȫ����
- ���й̼����뾭������ǩ��
- ��֤�̼���Դ��������
- ��������Ƶ�ʺ�Ȩ��

## 8. �����ų�

### 8.1 ��������

**Q: �����ű�ִ��ʧ��**
A: ����ļ�Ȩ�ޣ�ȷ���ű���ִ�У�
```bash
chmod +x build_version.sh release_version.sh
```

**Q: �汾�Ž�������**
A: ��� `main/version.h` �ļ���ʽ�Ƿ���ȷ��

**Q: OTA����ʧ��**
A: ����������ӡ�������״̬�Ͱ汾�����ԡ�

**Q: Git��ǩ����ʧ��**
A: ȷ����δ�ύ�ĸ��ģ����ֶ��ύ�汾���¡�

### 8.2 ������Ϣ
������ϸ��־��
```bash
# ����ʱ��ʾ��ϸ��Ϣ
./build_version.sh release 2>&1 | tee build.log

# �鿴�汾��Ϣ
grep "�汾��Ϣ" build.log
```

## 9. ��չ����

### 9.1 �Զ���汾���
�����޸� `check_for_updates()` ������ʵ���Զ���İ汾����߼���

### 9.2 ����������
�޸� `upload_to_server()` ������ʵ�����ض��������ļ��ɡ�

### 9.3 �汾ͳ��
��Ӱ汾�ֲ�ͳ�ƺ������ɹ��ʼ�ع��ܡ�

---

**ע�⣺** ��ʹ��˵�����ڰ汾Э�� v1.0.0�������汾���ܻ��и��¡� 