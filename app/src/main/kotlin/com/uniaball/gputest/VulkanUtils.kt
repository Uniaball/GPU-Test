package com.uniaball.gputest

import org.json.JSONObject

object VulkanUtils {

    private var loaded = false

    private fun ensureLoaded(): Boolean {
        if (loaded) return true
        return try {
            System.loadLibrary("vulkan_info")
            loaded = true
            true
        } catch (e: UnsatisfiedLinkError) {
            false
        }
    }

    private external fun nativeGetVulkanInfo(): String

    data class VulkanDeviceInfo(
        val deviceName: String,
        val deviceType: String,
        val apiVersion: String,
        val driverVersion: String,
        val vendorID: String,
        val deviceID: String,
        val extensions: List<String>
    )

    data class VulkanInfo(
        val supported: Boolean,
        val error: String?,
        val devices: List<VulkanDeviceInfo>
    )

    fun getVulkanInfo(): VulkanInfo {
        if (!ensureLoaded()) {
            return VulkanInfo(false, "无法加载 Vulkan 原生库 (libvulkan.so 不存在)", emptyList())
        }

        return try {
            val jsonStr = nativeGetVulkanInfo()
            parseVulkanInfo(jsonStr)
        } catch (e: Exception) {
            VulkanInfo(false, "获取 Vulkan 信息失败: ${e.message}", emptyList())
        }
    }

    private fun parseVulkanInfo(jsonStr: String): VulkanInfo {
        val root = JSONObject(jsonStr)
        val supported = root.getBoolean("supported")
        if (!supported) {
            return VulkanInfo(false, root.optString("error", "未知错误"), emptyList())
        }

        val devicesArr = root.getJSONArray("devices")
        val devices = mutableListOf<VulkanDeviceInfo>()

        for (i in 0 until devicesArr.length()) {
            val dev = devicesArr.getJSONObject(i)

            val extensions = mutableListOf<String>()
            val extArr = dev.getJSONArray("extensions")
            for (j in 0 until extArr.length()) {
                extensions.add(extArr.getString(j))
            }

            devices.add(VulkanDeviceInfo(
                deviceName = dev.getString("deviceName"),
                deviceType = dev.getString("deviceType"),
                apiVersion = dev.getString("apiVersion"),
                driverVersion = dev.getString("driverVersion"),
                vendorID = dev.getString("vendorID"),
                deviceID = dev.getString("deviceID"),
                extensions = extensions
            ))
        }

        return VulkanInfo(true, null, devices)
    }

    fun buildDetailText(info: VulkanInfo): String = buildDetailText(info, "")

    fun buildDetailText(info: VulkanInfo, extensionFilter: String): String {
        val sb = StringBuilder()
        val filter = extensionFilter.trim().lowercase()

        if (!info.supported) {
            sb.appendLine("Vulkan 不支持")
            sb.appendLine()
            sb.appendLine(info.error ?: "未知原因")
            return sb.toString()
        }

        for ((di, device) in info.devices.withIndex()) {
            if (info.devices.size > 1) {
                sb.appendLine("──────────────────────────────")
                sb.appendLine("设备 ${di + 1}/${info.devices.size}")
                sb.appendLine("──────────────────────────────")
            }

            sb.appendLine("设备名称: ${device.deviceName}")
            sb.appendLine("设备类型: ${device.deviceType}")
            sb.appendLine("API 版本: ${device.apiVersion}")
            sb.appendLine("驱动版本: ${device.driverVersion}")
            sb.appendLine("Vendor ID: ${device.vendorID}")
            sb.appendLine("Device ID: ${device.deviceID}")
            sb.appendLine()

            val exts = if (filter.isEmpty()) {
                device.extensions
            } else {
                device.extensions.filter { it.lowercase().contains(filter) }
            }
            sb.appendLine("扩展 (${exts.size}/${device.extensions.size}):")
            for (ext in exts) {
                sb.appendLine("  $ext")
            }
        }

        return sb.toString()
    }
}
