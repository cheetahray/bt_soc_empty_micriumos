# Update ADV Port - Nordic to Silicon Labs 移植層

## 🎯 快速開始

### 使用預定義選項 (推薦)

```c
#include "losstst_svc.h"

// 1. 初始化
uint8_t device_addr[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
losstst_svc_init(device_addr);

// 2. 設定參數 (使用預定義選項)
adv_param_t param = {
    .interval_min = PARAM_ADV_INT_MIN_0,  // 30ms
    .interval_max = PARAM_ADV_INT_MAX_0,  // 60ms
    .options = ADV_OPT_IDX_0,             // Extended + TX Power + Anonymous
    .tx_power = 50                        // 5.0 dBm
};

// 3. 啟動廣播
update_adv(0, &param, NULL, NULL);  // 使用預設廣播數據
```

### 可用的預定義選項

| 選項 | 說明 | 特性 |
|------|------|------|
| `ADV_OPT_IDX_0` | Extended 1M/2M PHY | TX Power + Anonymous + Extended |
| `ADV_OPT_IDX_1` | Extended 1M PHY | TX Power + Anonymous + Extended + NO_2M |
| `ADV_OPT_IDX_2` | Long Range (Coded) | TX Power + Anonymous + Extended + Coded PHY |
| `ADV_OPT_IDX_3` | Legacy BLE 4.x | Identity Address (不用 Extended) |

---

## 📚 文檔

### 核心文檔
- **[ADV_PARAM_FIELDS_EXPLAINED.md](ADV_PARAM_FIELDS_EXPLAINED.md)** - `adv_param_t` 各欄位完整說明
- **[NORDIC_TO_SILABS_MAPPING.md](NORDIC_TO_SILABS_MAPPING.md)** - Nordic 與 Silicon Labs API 對應關係

### Options 機制說明

**`options` 欄位是核心**，通過位元遮罩控制所有廣播特性：

```c
// 你只需設定 options，系統會自動處理：
param.options = BT_LE_ADV_OPT_EXT_ADV |        // ← 使用 extended advertising
                BT_LE_ADV_OPT_USE_TX_POWER |   // ← 包含 TX power 且呼叫 set_tx_power
                BT_LE_ADV_OPT_ANONYMOUS |      // ← 啟用匿名廣播
                BT_LE_ADV_OPT_CODED;           // ← 自動設定 PHY 為 Coded

// 內部會自動：
// 1. 解析 options → 設定 PHY (Coded)
// 2. 設定 TX Power (如果有 USE_TX_POWER)
// 3. 設定 flags (Anonymous, Include TX Power)
// 4. 選擇正確的 API (extended vs legacy)
// 5. 啟動廣播with正確的參數
```

---

## 🔍 重要欄位

### 必需欄位
- ✅ `interval_min` / `interval_max` - 廣播間隔
- ✅ `options` - **最重要**，控制所有行為

### 條件欄位
- ⚠️ `tx_power` - 當 `options` 包含 `BT_LE_ADV_OPT_USE_TX_POWER` 時必需

### 可選/自動欄位
- `primary_phy` / `secondary_phy` - 通常由 `options` 自動設定
- `id`, `sid`, `secondary_max_skip`, `peer` - Nordic 相容性，Silicon Labs 不使用

詳見 [ADV_PARAM_FIELDS_EXPLAINED.md](ADV_PARAM_FIELDS_EXPLAINED.md)

---

## 📋 完整範例

### 範例 1: 標準 Extended Advertising

```c
adv_param_t param = {
    .interval_min = PARAM_ADV_INT_MIN_0,
    .interval_max = PARAM_ADV_INT_MAX_0,
    .options = BT_LE_ADV_OPT_EXT_ADV | 
               BT_LE_ADV_OPT_USE_TX_POWER,
    .tx_power = 50  // 5.0 dBm
};

// 自訂廣播數據
adv_data_t my_adv_data[] = {
    {.type = BT_DATA_FLAGS, .data_len = 1, .data = (uint8_t[]){0x04}},
    {.type = BT_DATA_NAME_COMPLETE, .data_len = 7, .data = (uint8_t*)"MyDevice"}
};

update_adv(0, &param, my_adv_data, NULL);
```

### 範例 2: Long Range (最遠距離)

```c
adv_param_t param = {
    .interval_min = PARAM_ADV_INT_MIN_2,  // 90ms (Coded PHY 建議較長間隔)
    .interval_max = PARAM_ADV_INT_MAX_2,  // 180ms
    .options = ADV_OPT_IDX_2,             // Coded PHY
    .tx_power = 80                        // 8.0 dBm (Long Range 需要較高功率)
};

update_adv(2, &param, NULL, NULL);  // 使用預設數據
```

### 範例 3: Legacy BLE 4.x (相容舊設備)

```c
adv_param_t param = {
    .interval_min = PARAM_ADV_INT_MIN_0,
    .interval_max = PARAM_ADV_INT_MAX_0,
    .options = BT_LE_ADV_OPT_USE_IDENTITY,  // 只用身份地址，無 Extended
    .tx_power = 0
};

update_adv(3, &param, NULL, NULL);
```

---

## 🛠️ API 參考

### 主要函數

```c
// 初始化模組
int losstst_svc_init(const uint8_t *device_address);

// 更新/啟動廣播
int update_adv(uint8_t index, 
               const adv_param_t *adv_param,
               adv_data_t *adv_data,
               const adv_start_param_t *adv_start_param);

// 停止所有廣播
int stop_all_advertising(void);

// 設定 TX Power (Silicon Labs 專用)
int set_adv_tx_power(uint8_t index, int16_t power, int16_t *set_power);

// 取得狀態
const ext_adv_status_t* get_adv_status(uint8_t index);
```

### 輔助函數 (內部使用)

```c
// 轉換 Nordic options 為 Silicon Labs flags
uint8_t get_silabs_adv_flags(uint16_t nordic_options);

// 從 options 提取 PHY 設定
void get_phy_from_options(uint16_t nordic_options, 
                         uint8_t *primary_phy, 
                         uint8_t *secondary_phy);
```

---

## 🔧 內部處理流程

```
update_adv(index, param, data, start_param)
    │
    ├─→ 檢查是否已初始化
    │
    ├─→ 若未創建：platform_create_adv_set()
    │       ├─→ sl_bt_advertiser_create_set()
    │       ├─→ sl_bt_advertiser_set_timing()
    │       ├─→ 解析 options → 取得 PHY
    │       ├─→ sl_bt_extended_advertiser_set_phy()
    │       └─→ sl_bt_advertiser_set_tx_power() (如果需要)
    │
    ├─→ 若提供新參數：platform_update_adv_param()
    │       ├─→ 停止現有廣播
    │       ├─→ sl_bt_advertiser_set_timing()
    │       └─→ 更新 PHY 和 TX Power
    │
    ├─→ 設定廣播數據：platform_set_adv_data()
    │       └─→ sl_bt_extended_advertiser_set_data()
    │
    └─→ 啟動廣播：platform_start_adv()
            ├─→ 從 options 取得 flags
            ├─→ 判斷 extended vs legacy
            └─→ sl_bt_extended_advertiser_start() 或
                sl_bt_legacy_advertiser_start()
```

---

## ⚠️ 注意事項

1. **啟用 Silicon Labs API**: 
   - 目前 `PLATFORM_SILABS` 定義的實現返回 `-ENOTSUP`
   - 需要取消註釋 Silicon Labs API 呼叫並 include `sl_bt_api.h`

2. **TX Power 設定**:
   - 當使用 `BT_LE_ADV_OPT_USE_TX_POWER` 時，必須設定 `tx_power` 欄位
   - 單位是 0.1 dBm (例如：50 = 5.0 dBm)

3. **PHY 自動設定**:
   - 不要手動設定 `primary_phy`/`secondary_phy`
   - 使用 `options` 中的 `NO_2M` 和 `CODED` 標誌自動處理

4. **廣播類型**:
   - 有 `BT_LE_ADV_OPT_EXT_ADV` → Extended advertising
   - 沒有 → Legacy advertising (BLE 4.x)

---

## 📊 Options 標誌對照表

| Nordic 標誌 | 效果 | Silicon Labs 實現 |
|-------------|------|-------------------|
| `BT_LE_ADV_OPT_USE_TX_POWER` | 包含 TX Power | `sl_bt_advertiser_set_tx_power()` + `SL_BT_EXT_ADV_INCLUDE_TX_POWER` |
| `BT_LE_ADV_OPT_ANONYMOUS` | 匿名廣播 | `SL_BT_EXT_ADV_ANONYMOUS` |
| `BT_LE_ADV_OPT_EXT_ADV` | Extended advertising | 使用 `sl_bt_extended_advertiser_*` API |
| `BT_LE_ADV_OPT_NO_2M` | 不用 2M PHY | secondary_phy = `SL_BT_GAP_PHY_1M` |
| `BT_LE_ADV_OPT_CODED` | 使用 Coded PHY | primary/secondary_phy = `SL_BT_GAP_PHY_CODED` |
| `BT_LE_ADV_OPT_USE_IDENTITY` | 使用身份地址 | `sl_bt_advertiser_clear_random_address()` |
| `BT_LE_ADV_OPT_CONNECTABLE` | 可連接 | 設定 connection_mode |

---

## 🚀 移植步驟

### 從 Nordic 專案移植到 Silicon Labs：

1. ✅ 複製 `losstst_svc.h` 和 `losstst_svc.c`
2. ✅ 確認 `#define PLATFORM_SILABS`
3. ⚠️ Include Silicon Labs headers (`sl_bt_api.h`)
4. ⚠️ 取消註釋 Silicon Labs API 呼叫 (搜尋 `TODO: Uncomment`)
5. ✅ 使用 Nordic 程式碼的 `adv_param_t` 結構和 `options`
6. ✅ 呼叫 `update_adv()` 替代原有的 Nordic API

**不需要改變的**：你同事 Nordic 專案中的 `ADV_OPT_IDX_0/1/2/3` 定義可以直接使用！

---

## 📞 疑難排解

**Q: 為什麼返回 `-ENOTSUP`？**  
A: Silicon Labs API 呼叫被註釋了。取消註釋 `platform_create_adv_set()` 等函數中的實現。

**Q: `tx_power` 一定要設定嗎？**  
A: 只有當 `options` 包含 `BT_LE_ADV_OPT_USE_TX_POWER` 時才需要。

**Q: 可以手動設定 PHY 嗎？**  
A: 可以，但不推薦。建議使用 `options` 中的 `NO_2M` 和 `CODED` 標誌。

**Q: Legacy advertising 需要哪些設定？**  
A: 不要設定 `BT_LE_ADV_OPT_EXT_ADV`，只用 `BT_LE_ADV_OPT_USE_IDENTITY`。

---

## 📄 授權

本移植層是為了簡化 Nordic nRF52 到 Silicon Labs BG/MG 系列的 BLE advertising 程式碼移植而設計。
