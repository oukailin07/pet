# ����ιʳ���汾Э��淶

## 1. ����

���ĵ������˳���ιʳ����Ŀ�İ汾����Э�飬�����̼��汾��ͨ��Э��汾��OTA�������Ƶȹ淶��

## 2. �汾�Ÿ�ʽ

### 2.1 �̼��汾�Ÿ�ʽ
```
v<major>.<minor>.<patch>[-<build>][-<suffix>]
```

**��ʽ˵����**
- `major`: ���汾�ţ��ش��ܱ����ܹ�����ʱ����
- `minor`: �ΰ汾�ţ��¹������ʱ����
- `patch`: �����汾�ţ�Bug�޸�ʱ����
- `build`: �����ţ���ѡ������ʾ��������
- `suffix`: ��׺��ʶ����ѡ������ `alpha`��`beta`��`rc`��`stable`

**ʾ����**
- `v1.0.0` - ��һ���ȶ��汾
- `v1.2.3` - 1.2�汾�ĵ���������
- `v2.0.0-beta1` - 2.0�汾�ĵ�һ�����԰�
- `v1.5.2-20231201` - ���������ڵİ汾

### 2.2 ͨ��Э��汾��ʽ
```
<major>.<minor>
```

**��ʽ˵����**
- `major`: ���汾�ţ�Э�鲻���ݱ��ʱ����
- `minor`: �ΰ汾�ţ�Э���������չʱ����

**ʾ����**
- `1.0` - ��ʼЭ��汾
- `1.1` - Э����չ�汾
- `2.0` - �����ݵ�Э�����

## 3. �汾�������

### 3.1 �汾�ŵ�������

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

### 3.2 �汾������

#### �̼�������
- ͬһ���汾���ڣ���ȫ����
- ��ͬ���汾�ţ����ܲ����ݣ���Ҫ���⴦��

#### ͨ��Э�������
- �ͻ��˰汾 >= �������汾����ȫ����
- �ͻ��˰汾 < �������汾����Ҫ�����ͻ���

## 4. �汾��Ϣ����

### 4.1 �̼��汾��Ϣ�ṹ

```c
typedef struct {
    uint8_t major;           // ���汾��
    uint8_t minor;           // �ΰ汾��
    uint8_t patch;           // �����汾��
    uint16_t build;          // ������
    char suffix[16];         // �汾��׺
    char build_date[20];     // ��������
    char build_time[20];     // ����ʱ��
    char git_hash[16];       // Git�ύ��ϣ
} firmware_version_t;
```

### 4.2 ͨ��Э��汾��Ϣ

```c
typedef struct {
    uint8_t major;           // Э�����汾��
    uint8_t minor;           // Э��ΰ汾��
    char description[64];    // Э������
} protocol_version_t;
```

## 5. OTA����Э��

### 5.1 ��������

1. **�汾���**
   - �豸����ʱ����������浱ǰ�汾
   - ����������Ƿ��п��ø���

2. **����֪ͨ**
   - ��������������ָ��
   - �����°汾��Ϣ������URL

3. **�̼�����**
   - �豸��ָ��URL���ع̼�
   - ֧�ֶϵ�������У��

4. **�̼���֤**
   - У��̼�������
   - ��֤�汾������

5. **�̼���װ**
   - д��OTA����
   - ������������
   - �����豸

### 5.2 OTA��Ϣ��ʽ

#### �����������
```json
{
    "type": "version_check",
    "device_id": "device_001",
    "firmware_version": "v1.2.3",
    "protocol_version": "1.0",
    "hardware_version": "v1.0"
}
```

#### ���������Ӧ
```json
{
    "type": "version_check_result",
    "has_update": true,
    "latest_version": "v1.3.0",
    "download_url": "https://example.com/firmware/v1.3.0.bin",
    "file_size": 1048576,
    "checksum": "sha256:abc123...",
    "force_update": false,
    "description": "�޸���ιʳʱ��ͬ������"
}
```

#### ����ָ��
```json
{
    "type": "ota_update",
    "url": "https://example.com/firmware/v1.3.0.bin",
    "version": "v1.3.0",
    "checksum": "sha256:abc123...",
    "force": false
}
```

#### ����״̬����
```json
{
    "type": "ota_status",
    "device_id": "device_001",
    "status": "downloading", // downloading, installing, success, failed
    "progress": 75,          // ���ؽ��Ȱٷֱ�
    "error_code": 0,         // ������룬0��ʾ�޴���
    "error_message": ""      // ��������
}
```

## 6. �汾�ع�����

### 6.1 �Զ��ع�
- ����������ʧ��ʱ�Զ��ع�
- �������ʧ��ʱ�����ع�
- ��������������ð汾

### 6.2 �ֶ��ع�
- ͨ��WebSocketָ����ع�
- ֧��ָ���ع����ض��汾

### 6.3 �ع���Ϣ��ʽ
```json
{
    "type": "rollback_request",
    "target_version": "v1.2.3",
    "reason": "user_request"
}
```

## 7. �汾�����Լ��

### 7.1 �̼������Լ��
```c
bool check_firmware_compatibility(const firmware_version_t *current, 
                                 const firmware_version_t *target) {
    // ���汾�ű�����ͬ
    if (current->major != target->major) {
        return false;
    }
    
    // Ŀ��汾���ܵ��ڵ�ǰ�汾
    if (target->minor < current->minor) {
        return false;
    }
    
    if (target->minor == current->minor && target->patch < current->patch) {
        return false;
    }
    
    return true;
}
```

### 7.2 Э������Լ��
```c
bool check_protocol_compatibility(const protocol_version_t *client, 
                                 const protocol_version_t *server) {
    // ���汾�ű�����ͬ
    if (client->major != server->major) {
        return false;
    }
    
    // �ͻ��˰汾���ܵ��ڷ������汾
    if (client->minor < server->minor) {
        return false;
    }
    
    return true;
}
```

## 8. �汾��Ϣ�洢

### 8.1 �̼��汾��Ϣ�洢
- �洢��NVS����
- ������ǰ�汾�ͱ��ݰ汾��Ϣ
- ֧�ְ汾��ʷ��¼

### 8.2 �汾��Ϣ�ṹ
```c
typedef struct {
    firmware_version_t current_version;
    firmware_version_t backup_version;
    uint32_t install_time;
    uint32_t boot_count;
    bool is_stable;
} version_info_t;
```

## 9. �汾��������

### 9.1 �����汾
- ʹ�� `-alpha` �� `-beta` ��׺
- �������ڲ�����
- �����͸������豸

### 9.2 ��ѡ�汾
- ʹ�� `-rc` ��׺
- ����С��Χ����
- ͨ�����Ժ�ɷ�����ʽ��

### 9.3 ��ʽ�汾
- �޺�׺��ʹ�� `-stable` ��׺
- ������ֲ���
- �����͸������豸

## 10. �汾��غ�ͳ��

### 10.1 �汾�ֲ�ͳ��
- ͳ�Ƹ��汾�豸����
- ��ذ汾�����ɹ���
- ���ٰ汾�ȶ���ָ��

### 10.2 ����ʧ�ܷ���
- ��¼����ʧ��ԭ��
- ����ʧ��ģʽ
- �Ż���������

## 11. ��ȫ����

### 11.1 �̼�ǩ��
- ���й̼����뾭������ǩ��
- ��֤�̼���Դ��������
- ��ֹ����̼���װ

### 11.2 ����Ȩ�޿���
- ��������Ƶ��
- ֧������������
- �ؼ��豸��Ҫ������Ȩ

## 12. ʵʩָ��

### 12.1 ����ʵ��
1. �� `main/version.h` �ж���汾��Ϣ
2. �� `main/version.c` ��ʵ�ְ汾������
3. ��WebSocket�ͻ����м��ɰ汾���
4. ��OTAģ����ʵ�ְ汾��֤

### 12.2 �����ļ�
1. �� `CMakeLists.txt` �ж���汾��
2. �� `sdkconfig` �����ð汾���ѡ��
3. �ڹ����ű����Զ����ɰ汾��Ϣ

### 12.3 ������֤
1. ��Ԫ���԰汾������
2. ���ɲ���OTA��������
3. �����Բ��Բ�ͬ�汾��ͨ��

## 13. �����¼

| �汾 | ���� | ������� | ���� |
|------|------|----------|------|
| 1.0.0 | 2024-01-01 | ��ʼ�汾Э�� | �����Ŷ� |

---

**ע�⣺** ��Э��汾Ϊ v1.0.0�������汾���ڴ��ĵ������Ͻ��и��º����ơ� 