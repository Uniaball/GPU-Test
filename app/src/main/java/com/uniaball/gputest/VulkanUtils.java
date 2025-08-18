package com.uniaball.gputest;

import android.app.Activity;
import android.content.pm.PackageManager;
import android.os.Build;

public class VulkanUtils {
    public static boolean isSupported(Activity activity) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            return activity.getPackageManager().hasSystemFeature(
                PackageManager.FEATURE_VULKAN_HARDWARE_LEVEL
            );
        }
        return false;
    }
    
    public static boolean isSupportedVulkan12(Activity activity) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            return activity.getPackageManager().hasSystemFeature(
                PackageManager.FEATURE_VULKAN_HARDWARE_LEVEL,
                2
            );
        }
        return false;
    }
}
