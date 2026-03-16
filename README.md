# Foco Inteligente WS2812 + ESP32

**Autor**: Rodrigo Calle Condori
**Fecha**: Marzo 2026
**Versión**: 1.0.0

Control de tira LED WS2812 vía BLE con ESP32. Proyecto desarrollado como 
base para un producto comercial de iluminación inteligente.

## ✨ Características
- Control de color RGB (16 millones de colores)
- Ajuste de brillo (0-100%)
- 3 modos de operación (sólido, reservado para efectos)
- Comunicación BLE con app móvil (nRF Connect / Flutter)

## 📦 Componentes
- `led_controller`: Control de LEDs WS2812 con RMT
- `ble_foco`: Servicio BLE personalizado


## 🔧 Hardware Requerido
- ESP32 (cualquier variante)
- Tira de LEDs WS2812 (24 LEDs recomendado)
- Fuente de alimentación 5V/2A (para 24 LEDs)
- MOSFET convertidor de nivel (opcional, recomendado)

## 📱 Uso con nRF Connect
1. Conectar ESP32 a alimentación
2. Escanear dispositivos BLE
3. Conectar a "ESP_FOCO_TEST"
4. Escribir en características:
   - **Color** (UUID 0xFF01): 3 bytes [R, G, B]
   - **Brillo** (UUID 0xFF02): 1 byte (0-100)
   - **Modo** (UUID 0xFF03): 1 byte (reservado)

## 🚀 Compilación
```bash
idf.py set-target esp32
idf.py menuconfig  # Opcional: configurar opciones
idf.py build
idf.py flash monitor
```

##📬 Contacto
rodrigocallecondori@gmail.com

## 📝 Licencia
Copyright (c) 2026 [Rodrigo Calle Condori]. Todos los derechos reservados.

Este proyecto es de propiedad privada y no puede ser distribuido 
ni utilizado sin autorización explícita del autor.