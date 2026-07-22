package com.uniaball.gputest

import android.app.Activity
import android.content.pm.PackageManager
import android.os.Build

object VulkanUtils {
    fun isSupported(activity: Activity): Boolean {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            return activity.packageManager.hasSystemFeature(PackageManager.FEATURE_VULKAN_HARDWARE_LEVEL)
        }
        return false
    }

    fun isSupportedVulkan12(activity: Activity): Boolean {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            return activity.packageManager.hasSystemFeature(PackageManager.FEATURE_VULKAN_HARDWARE_LEVEL, 2)
        }
        return false
    }
}
