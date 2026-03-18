# Foco Inteligente WS2812 + ESP32

**Autor**: Rodrigo Calle Condori  
**Fecha**: Marzo 2026  
**Versión**: 1.3.0  

Control de tira LED WS2812 vía BLE con ESP32. Proyecto desarrollado con arquitectura profesional y modular como base para un producto comercial de iluminación inteligente.

### ✅ Implementado
- Control de color RGB (16 millones de colores)
- Control de temperatura de color blanco (2700K - 6500K)
- Ajuste de brillo global (0-100%)
- **Modos y escenas preprogramadas:**
  - 🌈 Arcoíris (transición de colores cíclica)
  - 🌅 Atardecer (transición de temperatura blanca)
  - 🎉 Fiesta (colores aleatorios rápidos)
  - 😌 Relajación (transiciones suaves de colores pastel)
  - 🌙 Nocturno (luz cálida tenue)
  - ⛈️ Tormenta (efecto de relámpagos aleatorios)
- Comunicación BLE con app móvil (nRF Connect)
- Arquitectura modular y escalable

### ⏳ En desarrollo
- Sincronización musical
- Conexión WiFi y asistentes de voz

## 🏗️ Arquitectura del Proyecto

```mermaid
graph TD
    main[main.c<br/>orquestador de componentes] --> color[led_color<br/>RGB]
    main --> white[led_white<br/>temp. color]
    main --> modes[led_modes<br/>escenas]
    main --> music[led_music<br/>ritmo]
    
    color --> controller[led_controller<br/>hardware RMT]
    white --> controller
    modes --> controller
    music --> controller
    
    controller --> encoder[led_strip_encoder<br/>driver RMT]
```

## 📦 Componentes Implementados

| Componente | Descripción | Estado |
|:---|:---|:---|
| **`led_controller`** | Capa base de hardware. Controla LEDs WS2812 vía RMT, gestiona brillo global y buffer de píxeles. | ✅ Estable |
| **`led_color`** | Módulo de alto nivel para control RGB. Proporciona API intuitiva para colores sólidos. | ✅ Estable |
| **`led_white`** | Módulo para control de temperatura de color blanco (2700K-6500K). Convierte Kelvin a RGB usando algoritmo de Tanner Helland. | ✅ Estable |
| **`led_modes`** | Efectos y escenas preprogramadas con velocidad ajustable. | ✅ Estable |
| **`led_strip_encoder`** | Driver de bajo nivel para WS2812. Convierte bytes a señales RMT precisas. | ✅ Estable |
| **`ble_foco`** | Servicio BLE personalizado con UUIDs para color, brillo, modo y temperatura blanca. | ✅ Estable |
| **`led_music`** | Sincronización con ritmo musical. | ⏳ Futuro |

## 🎮 Modos de Operación

| Modo | ID | Descripción | Comportamiento |
|:---|:---|:---|:---|
| **Sólido** | 0 | Color fijo | Muestra el último color seleccionado (por color o blanco) |
| **Arcoíris** | 1 | 🌈 | Transición cíclica de colores |
| **Atardecer** | 2 | 🌅 | Transición suave entre 2700K y 6500K |
| **Fiesta** | 3 | 🎉 | Colores aleatorios rápidos |
| **Relajación** | 4 | 😌 | Transiciones lentas de colores pastel |
| **Nocturno** | 5 | 🌙 | Luz cálida (2700K) a bajo brillo |
| **Tormenta** | 6 | ⛈️ | Relámpagos blancos aleatorios |

## 🔧 Hardware Requerido
- ESP32 (cualquier variante)
- Tira de LEDs WS2812 (24 LEDs recomendado)
- Fuente de alimentación 5V/2A-3A (para 24 LEDs a máximo brillo)
- MOSFET convertidor de nivel (3.3V → 5V para datos)
- Resistencia 330Ω-470Ω en línea de datos

## 📱 Uso con nRF Connect

### 1. Conectar ESP32 a alimentación
### 2. Escanear dispositivos BLE
### 3. Conectar a **"ESP_FOCO_TEST"**
### 4. Escribir en características:

| Característica | UUID | Formato | Rango | Ejemplo |
|:---|:---|:---|:---|:---|
| **Color** | `0xFF01` | 3 bytes [R, G, B] | 0-255 cada uno | `FF0000` = Rojo |
| **Brillo** | `0xFF02` | 1 byte | 0-100 | `64` = 100% |
| **Modo** | `0xFF03` | 2 bytes [modo, velocidad] | modo: 0-6, velocidad: 0-100 | `01 32` = Arcoíris 50% |
| **Blanco** | `0xFF04` | 2 bytes (little-endian) | 2700-6500K | `8C 0A` = 2700K |

### 📝 Tabla de modos para nRF Connect:

| Modo | Valor (1er byte) | Velocidad (2do byte) | Ejemplo |
|:---|:---|:---|:---|
| Sólido | `00` | `00` | `00 00` |
| Arcoíris | `01` | `32` (50) | `01 32` |
| Atardecer | `02` | `32` | `02 32` |
| Fiesta | `03` | `32` | `03 32` |
| Relajación | `04` | `32` | `04 32` |
| Nocturno | `05` | `32` | `05 32` |
| Tormenta | `06` | `32` | `06 32` |

### 📝 Notas sobre temperatura de color (modo led white):
- **2700K**: Muy cálido (ámbar) - `8C 0A` en hexadecimal
- **4000K**: Neutro - `A0 0F` en hexadecimal
- **6500K**: Muy frío (azul) - `64 19` en hexadecimal

Los valores se envían en formato **little-endian** (byte bajo primero):
- 2700K = 0x0A8C → enviar `8C 0A`
- 4000K = 0x0FA0 → enviar `A0 0F`
- 6500K = 0x1964 → enviar `64 19`

## 🚀 Compilación y Flash

```bash
# Configurar target
idf.py set-target esp32

# (Opcional) Configurar opciones
idf.py menuconfig

# Compilar
idf.py build

# Grabar y monitorear
idf.py flash monitor

# Para salir del monitor: Ctrl + ]
```
## 📂 Estructura del Proyecto

```
foco_inteligente/
├── components/
│   ├── led_controller/          # Capa base de hardware
│   │   ├── include/
│   │   │   └── led_controller.h
│   │   ├── led_controller.c
│   │   └── CMakeLists.txt
│   ├── led_color/                # Control RGB
│   │   ├── include/
│   │   │   └── led_color.h
│   │   ├── led_color.c
│   │   └── CMakeLists.txt
│   ├── led_white/                 # Control blanco Kelvin
│   │   ├── include/
│   │   │   └── led_white.h
│   │   ├── led_white.c
│   │   └── CMakeLists.txt
│   ├── led_modes/                 # Modos y escenas
│   │   ├── include/
│   │   │   └── led_modes.h
│   │   ├── led_modes.c
│   │   └── CMakeLists.txt
│   ├── ble_foco/                   # Servicio BLE
│   │   ├── include/
│   │   │   └── ble_foco.h
│   │   ├── ble_foco.c
│   │   └── CMakeLists.txt
│   └── led_strip_encoder/        # Driver WS2812
│       ├── led_strip_encoder.c
│       └── led_strip_encoder.h
├── main/
│   ├── CMakeLists.txt
│   └── main.c                    # Orquestador principal
├── CMakeLists.txt                 # Proyecto raíz
└── README.md
```


## 📬 Contacto
rodrigocallecondori@gmail.com

# 📝 Licencia
Copyright (c) 2026 Rodrigo Calle Condori. Todos los derechos reservados.

Este proyecto es de propiedad privada y no puede ser distribuido, modificado ni utilizado sin autorización explícita del autor. Todos los derechos de propiedad intelectual pertenecen al autor.