#include "ble_foco.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_common_api.h"
#include "nvs_flash.h"

static const char *TAG = "BLE_FOCO";


// ==================== CONFIGURACIÓN ====================
#define PROFILE_NUM                 1
#define PROFILE_APP_IDX             0
#define ESP_APP_ID                  0x55
#define SAMPLE_DEVICE_NAME          "ESP_FOCO_TEST"
#define SVC_INST_ID                 0

#define PREPARE_BUF_MAX_SIZE        1024
#define CHAR_DECLARATION_SIZE       (sizeof(uint8_t))

#define ADV_CONFIG_FLAG             (1 << 0)
#define SCAN_RSP_CONFIG_FLAG        (1 << 1)

// ==================== HANDLES Y ENUMS ====================
enum {
    IDX_SVC,
    IDX_CHAR_COLOR_DECL,
    IDX_CHAR_COLOR_VAL,
    IDX_CHAR_BRILLO_DECL,
    IDX_CHAR_BRILLO_VAL,
    IDX_CHAR_MODO_DECL,
    IDX_CHAR_MODO_VAL,
    FOCO_IDX_NB
};

static uint16_t foco_handle_table[FOCO_IDX_NB];
static uint8_t adv_config_done = 0;

// ==================== VALORES ACTUALES ====================
static uint8_t current_color[3] = {255, 255, 255};
static uint8_t current_brightness = 100;
static uint8_t current_mode = 0;

// ==================== CALLBACKS DE USUARIO ====================
static ble_foco_callbacks_t user_callbacks = {0};

// ==================== UUIDs ====================
static const uint16_t GATTS_SERVICE_UUID_FOCO      = 0x00FF;
static const uint16_t GATTS_CHAR_UUID_COLOR        = 0xFF01;
static const uint16_t GATTS_CHAR_UUID_BRILLO       = 0xFF02;
static const uint16_t GATTS_CHAR_UUID_MODO         = 0xFF03;

static const uint16_t primary_service_uuid         = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid   = ESP_GATT_UUID_CHAR_DECLARE;
static const uint8_t char_prop_read_write = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ;

// ==================== PREPARE WRITE ====================
typedef struct {
    uint8_t *prepare_buf;
    int prepare_len;
} prepare_type_env_t;
static prepare_type_env_t prepare_write_env;

// ==================== ADVERTISING DATA ====================
static uint8_t service_uuid[16] = {
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
    0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp        = false,
    .include_name        = true,
    .include_txpower     = true,
    .min_interval        = 0x20,
    .max_interval        = 0x40,
    .appearance          = 0x00,
    .manufacturer_len    = 0,
    .p_manufacturer_data = NULL,
    .service_data_len    = 0,
    .p_service_data      = NULL,
    .service_uuid_len    = sizeof(service_uuid),
    .p_service_uuid      = service_uuid,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp        = true,
    .include_name        = true,
    .include_txpower     = true,
    .min_interval        = 0x20,
    .max_interval        = 0x40,
    .appearance          = 0x00,
    .manufacturer_len    = 0,
    .p_manufacturer_data = NULL,
    .service_data_len    = 0,
    .p_service_data      = NULL,
    .service_uuid_len    = sizeof(service_uuid),
    .p_service_uuid      = service_uuid,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min         = 0x20,
    .adv_int_max         = 0x40,
    .adv_type            = ADV_TYPE_IND,
    .own_addr_type       = BLE_ADDR_TYPE_PUBLIC,
    .channel_map         = ADV_CHNL_ALL,
    .adv_filter_policy   = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

// ==================== TABLA GATT ====================
static const esp_gatts_attr_db_t foco_gatt_db[FOCO_IDX_NB] = {
    [IDX_SVC] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
         sizeof(uint16_t), sizeof(GATTS_SERVICE_UUID_FOCO), 
         (uint8_t *)&GATTS_SERVICE_UUID_FOCO}
    },
    [IDX_CHAR_COLOR_DECL] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
         CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, 
         (uint8_t *)&char_prop_read_write}
    },
    [IDX_CHAR_COLOR_VAL] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_COLOR, 
         ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
         3, sizeof(current_color), (uint8_t *)current_color}
    },
    [IDX_CHAR_BRILLO_DECL] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
         CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, 
         (uint8_t *)&char_prop_read_write}
    },
    [IDX_CHAR_BRILLO_VAL] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_BRILLO, 
         ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
         1, sizeof(current_brightness), (uint8_t *)&current_brightness}
    },
    [IDX_CHAR_MODO_DECL] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
         CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, 
         (uint8_t *)&char_prop_read_write}
    },
    [IDX_CHAR_MODO_VAL] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_MODO, 
         ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
         1, sizeof(current_mode), (uint8_t *)&current_mode}
    },
};

// ==================== FUNCIONES AUXILIARES ====================
static void example_prepare_write_event_env(esp_gatt_if_t gatts_if,
                                           prepare_type_env_t *prepare_write_env,
                                           esp_ble_gatts_cb_param_t *param)
{
    ESP_LOGI(TAG, "prepare write, handle = %d, value len = %d",
             param->write.handle, param->write.len);
    
    esp_gatt_status_t status = ESP_GATT_OK;
    if (param->write.offset > PREPARE_BUF_MAX_SIZE) {
        status = ESP_GATT_INVALID_OFFSET;
    } else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE) {
        status = ESP_GATT_INVALID_ATTR_LEN;
    }
    
    if (status == ESP_GATT_OK && prepare_write_env->prepare_buf == NULL) {
        prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE);
        prepare_write_env->prepare_len = 0;
        if (prepare_write_env->prepare_buf == NULL) {
            ESP_LOGE(TAG, "No memory for prepare buffer");
            status = ESP_GATT_NO_RESOURCES;
        }
    }

    if (param->write.need_rsp) {
        esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
        if (gatt_rsp != NULL) {
            gatt_rsp->attr_value.len = param->write.len;
            gatt_rsp->attr_value.handle = param->write.handle;
            gatt_rsp->attr_value.offset = param->write.offset;
            gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
            memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
            esp_err_t response_err = esp_ble_gatts_send_response(gatts_if,
                                        param->write.conn_id,
                                        param->write.trans_id,
                                        status, gatt_rsp);
            if (response_err != ESP_OK) {
                ESP_LOGE(TAG, "Send response error");
            }
            free(gatt_rsp);
        } else {
            ESP_LOGE(TAG, "malloc failed");
            status = ESP_GATT_NO_RESOURCES;
        }
    }
    
    if (status != ESP_GATT_OK) {
        return;
    }
    
    memcpy(prepare_write_env->prepare_buf + param->write.offset,
           param->write.value, param->write.len);
    prepare_write_env->prepare_len += param->write.len;
}

static void example_exec_write_event_env(prepare_type_env_t *prepare_write_env,
                                        esp_ble_gatts_cb_param_t *param)
{
    if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC &&
        prepare_write_env->prepare_buf) {
        ESP_LOG_BUFFER_HEX(TAG, prepare_write_env->prepare_buf,
                          prepare_write_env->prepare_len);
    } else {
        ESP_LOGI(TAG, "ESP_GATT_PREP_WRITE_CANCEL");
    }
    
    if (prepare_write_env->prepare_buf) {
        free(prepare_write_env->prepare_buf);
        prepare_write_env->prepare_buf = NULL;
    }
    prepare_write_env->prepare_len = 0;
}

// ==================== PERFILES ====================
struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
                                       esp_gatt_if_t gatts_if,
                                       esp_ble_gatts_cb_param_t *param);

static struct gatts_profile_inst foco_profile_tab[PROFILE_NUM] = {
    [PROFILE_APP_IDX] = {
        .gatts_cb = gatts_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,
    },
};

// ==================== GAP EVENT HANDLER ====================
static void gap_event_handler(esp_gap_ble_cb_event_t event,
                             esp_ble_gap_cb_param_t *param)
{
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            adv_config_done &= (~ADV_CONFIG_FLAG);
            if (adv_config_done == 0) esp_ble_gap_start_advertising(&adv_params);
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
            adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
            if (adv_config_done == 0) esp_ble_gap_start_advertising(&adv_params);
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            ESP_LOGI(TAG, "advertising start %s", 
                     param->adv_start_cmpl.status == ESP_BT_STATUS_SUCCESS ? "ok" : "failed");
            break;
        default:
            break;
    }
}

// ==================== GATT PROFILE EVENT HANDLER ====================
static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
                                       esp_gatt_if_t gatts_if,
                                       esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
        case ESP_GATTS_REG_EVT: {
            esp_ble_gap_set_device_name(SAMPLE_DEVICE_NAME);
            esp_ble_gap_config_adv_data(&adv_data);
            adv_config_done |= ADV_CONFIG_FLAG;
            esp_ble_gap_config_adv_data(&scan_rsp_data);
            adv_config_done |= SCAN_RSP_CONFIG_FLAG;
            esp_ble_gatts_create_attr_tab(foco_gatt_db, gatts_if, FOCO_IDX_NB, SVC_INST_ID);
            break;
        }

        case ESP_GATTS_READ_EVT:
            ESP_LOGI(TAG, "Lectura en handle %d", param->read.handle);
            break;

        case ESP_GATTS_WRITE_EVT: {
            if (param->write.is_prep) {
                example_prepare_write_event_env(gatts_if, &prepare_write_env, param);
                break;
            }
            
            uint16_t handle = param->write.handle;
            uint8_t *data = param->write.value;
            size_t len = param->write.len;

            if (handle == foco_handle_table[IDX_CHAR_COLOR_VAL] && len >= 3) {
                current_color[0] = data[0];
                current_color[1] = data[1];
                current_color[2] = data[2];
                if (user_callbacks.on_color_change) {
                    user_callbacks.on_color_change(data[0], data[1], data[2]);
                }
            }
            else if (handle == foco_handle_table[IDX_CHAR_BRILLO_VAL] && len >= 1) {
                if (data[0] <= 100) {
                    current_brightness = data[0];
                    if (user_callbacks.on_brightness_change) {
                        user_callbacks.on_brightness_change(data[0]);
                    }
                }
            }
            else if (handle == foco_handle_table[IDX_CHAR_MODO_VAL] && len >= 1) {
                current_mode = data[0];
                if (user_callbacks.on_mode_change) {
                    user_callbacks.on_mode_change(data[0]);
                }
            }

            if (param->write.need_rsp) {
                esp_ble_gatts_send_response(gatts_if, param->write.conn_id,
                                           param->write.trans_id, ESP_GATT_OK, NULL);
            }
            break;
        }

        case ESP_GATTS_EXEC_WRITE_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_EXEC_WRITE_EVT");
            example_exec_write_event_env(&prepare_write_env, param);
            break;

        case ESP_GATTS_MTU_EVT:
            ESP_LOGI(TAG, "📦 MTU: %d", param->mtu.mtu);
            break;

        case ESP_GATTS_CONF_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_CONF_EVT, status = %d, handle %d",
                    param->conf.status, param->conf.handle);
            break;

        case ESP_GATTS_START_EVT:
            ESP_LOGI(TAG, "Servicio iniciado, handle %d",
                    param->start.service_handle);
            break;

        case ESP_GATTS_CONNECT_EVT: {
            ESP_LOGI(TAG, "Cliente conectado");
            if (user_callbacks.on_connect) user_callbacks.on_connect();
            
            esp_ble_conn_update_params_t conn_params = {0};
            memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
            conn_params.latency = 0;
            conn_params.max_int = 0x20;
            conn_params.min_int = 0x10;
            conn_params.timeout = 400;
            esp_ble_gap_update_conn_params(&conn_params);
            break;
        }

        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(TAG, "Cliente desconectado");
            if (user_callbacks.on_disconnect) user_callbacks.on_disconnect();
            esp_ble_gap_start_advertising(&adv_params);
            break;

        case ESP_GATTS_CREAT_ATTR_TAB_EVT: {
            if (param->add_attr_tab.status != ESP_GATT_OK) break;
            if (param->add_attr_tab.num_handle != FOCO_IDX_NB) break;
            
            memcpy(foco_handle_table, param->add_attr_tab.handles, sizeof(foco_handle_table));
            esp_ble_gatts_start_service(foco_handle_table[IDX_SVC]);
            
            ESP_LOGI(TAG, "✅ Servicio BLE iniciado");
            ESP_LOGI(TAG, "Handles - SVC:%d COLOR:%d BRILLO:%d MODO:%d",
                     foco_handle_table[IDX_SVC],
                     foco_handle_table[IDX_CHAR_COLOR_VAL],
                     foco_handle_table[IDX_CHAR_BRILLO_VAL],
                     foco_handle_table[IDX_CHAR_MODO_VAL]);
            break;
        }

        // Capturan eventos que el stack BLE podría generar pero que no necesitas procesar
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
    case ESP_GATTS_STOP_EVT:
    case ESP_GATTS_UNREG_EVT:
    case ESP_GATTS_DELETE_EVT:
        ESP_LOGD(TAG, "Evento no manejado: %d", event);
        break;

    default:
    ESP_LOGD(TAG, "Evento desconocido: %d", event);
        break;
    }
}

// ==================== GATT EVENT HANDLER PRINCIPAL ====================
static void gatts_event_handler(esp_gatts_cb_event_t event,
                               esp_gatt_if_t gatts_if,
                               esp_ble_gatts_cb_param_t *param)
{
    if (event == ESP_GATTS_REG_EVT && param->reg.status == ESP_GATT_OK) {
        foco_profile_tab[PROFILE_APP_IDX].gatts_if = gatts_if;
    }

    for (int idx = 0; idx < PROFILE_NUM; idx++) {
        if (gatts_if == ESP_GATT_IF_NONE || gatts_if == foco_profile_tab[idx].gatts_if) {
            if (foco_profile_tab[idx].gatts_cb) {
                foco_profile_tab[idx].gatts_cb(event, gatts_if, param);
            }
        }
    }
}

// ==================== FUNCIONES PÚBLICAS ====================
esp_err_t ble_foco_init(void)
{
    ESP_LOGI(TAG, "Inicializando módulo BLE del foco");

    // Inicializar NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Liberar memoria de Bluetooth Classic
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    // Inicializar controlador Bluetooth
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(TAG, "Error inicializando controlador BT: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(TAG, "Error habilitando controlador BT: %s", esp_err_to_name(ret));
        return ret;
    }

    // Inicializar Bluedroid
    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(TAG, "Error inicializando bluedroid: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(TAG, "Error habilitando bluedroid: %s", esp_err_to_name(ret));
        return ret;
    }

    // Registrar callbacks
    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret) {
        ESP_LOGE(TAG, "Error registrando callback GATT: %x", ret);
        return ret;
    }

    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret) {
        ESP_LOGE(TAG, "Error registrando callback GAP: %x", ret);
        return ret;
    }

    // Registrar aplicación
    ret = esp_ble_gatts_app_register(ESP_APP_ID);
    if (ret) {
        ESP_LOGE(TAG, "Error registrando app GATT: %x", ret);
        return ret;
    }

    // Configurar MTU local
    esp_ble_gatt_set_local_mtu(500);

    ESP_LOGI(TAG, "✅ Módulo BLE inicializado correctamente");
    return ESP_OK;
}

void ble_foco_register_callbacks(const ble_foco_callbacks_t *cbs)
{
    if (cbs) {
        memcpy(&user_callbacks, cbs, sizeof(ble_foco_callbacks_t));
        ESP_LOGI(TAG, "Callbacks registrados");
    }
}

esp_err_t ble_foco_update_color(uint8_t r, uint8_t g, uint8_t b)
{
    current_color[0] = r;
    current_color[1] = g;
    current_color[2] = b;
    
    // Actualizar valor en el servidor GATT
    esp_ble_gatts_set_attr_value(foco_handle_table[IDX_CHAR_COLOR_VAL],
                                 3, current_color);
    return ESP_OK;
}

esp_err_t ble_foco_update_brightness(uint8_t brightness)
{
    if (brightness > 100) brightness = 100;
    current_brightness = brightness;
    esp_ble_gatts_set_attr_value(foco_handle_table[IDX_CHAR_BRILLO_VAL],
                                 1, &current_brightness);
    return ESP_OK;
}

esp_err_t ble_foco_update_mode(uint8_t mode)
{
    current_mode = mode;
    esp_ble_gatts_set_attr_value(foco_handle_table[IDX_CHAR_MODO_VAL],
                                 1, &current_mode);
    return ESP_OK;
}

void ble_foco_get_current(uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *brightness, uint8_t *mode)
{
    if (r) *r = current_color[0];
    if (g) *g = current_color[1];
    if (b) *b = current_color[2];
    if (brightness) *brightness = current_brightness;
    if (mode) *mode = current_mode;
}