package com.uniaball.gputest

object VulkanTestUtils {

    private var loaded = false

    private fun ensureLoaded(): Boolean {
        if (loaded) return true
        return try {
            System.loadLibrary("vulkan_test")
            loaded = true
            true
        } catch (e: UnsatisfiedLinkError) {
            false
        }
    }

    fun isSupported(): Boolean = ensureLoaded()

    // Native methods
    private external fun nativeInit(surface: Any, width: Int, height: Int, sphereCount: Int): Boolean
    private external fun nativeDrawFrame()
    private external fun nativeGetFPS(): Float
    private external fun nativeGetFrameCount(): Long
    private external fun nativeGetDeviceInfo(): String
    private external fun nativeCleanup()
    private external fun nativeGetLatestFPS(): Float

    data class DeviceInfo(
        val deviceName: String,
        val apiVersion: String,
        val width: Int,
        val height: Int,
        val sphereCount: Int
    )

    fun init(surface: Any, width: Int, height: Int, sphereCount: Int): Boolean {
        if (!ensureLoaded()) return false
        return nativeInit(surface, width, height, sphereCount)
    }

    fun drawFrame() {
        nativeDrawFrame()
    }

    fun getFPS(): Float = nativeGetFPS()

    fun getFrameCount(): Long = nativeGetFrameCount()

    fun getLatestFPS(): Float = nativeGetLatestFPS()

    fun getDeviceInfo(): DeviceInfo {
        val jsonStr = nativeGetDeviceInfo()
        val json = org.json.JSONObject(jsonStr)
        return DeviceInfo(
            deviceName = json.getString("deviceName"),
            apiVersion = json.getString("apiVersion"),
            width = json.getInt("width"),
            height = json.getInt("height"),
            sphereCount = json.getInt("sphereCount")
        )
    }

    fun cleanup() {
        nativeCleanup()
    }
}
