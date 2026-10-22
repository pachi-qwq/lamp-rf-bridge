// SPDX-License-Identifier: MIT
#pragma once
#include "lamp_controller.h"
namespace network_app {
/**
 * @brief Initialize NVS, saved Wi-Fi and HTTP without transmitting.
 * 初始化 NVS、已存 Wi-Fi 与 HTTP，不发射。
 * @param controller Caller-owned controller that outlives the network app. / 生命周期长于网络应用的控制器。
 * @return False on storage initialization/persistence failure. / 存储初始化或持久化失败时 false。
 */
bool begin(lamp::Controller& controller);
/**
 * @brief Service HTTP and Wi-Fi reconnection frequently from the Arduino task.
 * 在 Arduino 任务频繁处理 HTTP 与 Wi-Fi 重连。
 */
void loop();
// Print access details on an access command or first unconfigured boot, never in the public status API.
// 首次未配网启动或 access 命令通过 USB 输出访问资料；不进入公开状态 API。
/**
 * @brief Print private access details over physical USB; never publish this output.
 * 通过物理 USB 打印私密访问资料；禁止公开输出。
 */
void printAccess();
/**
 * @brief Open the password-protected setup AP if not already active.
 * 尚未开启时打开有密码的配网热点。
 */
void enableSetup();
}  // namespace network_app
