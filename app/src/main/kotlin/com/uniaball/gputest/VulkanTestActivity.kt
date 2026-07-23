package com.uniaball.gputest

import android.graphics.Color
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.Gravity
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.View
import android.view.ViewGroup
import android.view.WindowManager
import android.view.animation.AlphaAnimation
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.TextView
import android.widget.Toast
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.updateLayoutParams
import com.google.android.material.card.MaterialCardView

class VulkanTestActivity : AppCompatActivity() {
    companion object {
        private const val TAG = "VulkanTestActivity"
    }

    private var surfaceView: SurfaceView? = null
    private lateinit var fpsTextView: TextView
    private lateinit var infoTextView: TextView
    private lateinit var performanceTextView: TextView
    private lateinit var performanceCardView: MaterialCardView
    private lateinit var apiVersionTextView: TextView
    private var frameCount = 0
    private var lastTime: Long = 0
    private var startTime: Long = 0
    private val handler = Handler(Looper.getMainLooper())

    private var isTesting = false
    private var testStartTime: Long = 0
    private var testFrameCount = 0
    private val testDuration = 5000L

    private var sphereCount: Int = 0

    private var isRendering = false
    private var renderThread: Thread? = null
    private var surfaceReady = false
    private var vulkanInitialized = false

    override fun onCreate(savedInstanceState: Bundle?) {
        enableEdgeToEdge()
        super.onCreate(savedInstanceState)

        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)

        sphereCount = SettingsActivity.getSphereCount(this)

        // SurfaceView
        surfaceView = SurfaceView(this).apply {
            id = View.generateViewId()
            layoutParams = FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
            )
            holder.addCallback(object : SurfaceHolder.Callback {
                override fun surfaceCreated(holder: SurfaceHolder) {
                    surfaceReady = true
                    startRendering()
                }
                override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {}
                override fun surfaceDestroyed(holder: SurfaceHolder) {
                    surfaceReady = false
                    stopRendering()
                }
            })
        }

        // FPS overlay card
        fpsTextView = TextView(this).apply {
            id = View.generateViewId()
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            )
            text = "FPS: 0 | Time: 0s"
            setTextAppearance(androidx.appcompat.R.style.TextAppearance_AppCompat_Body1)
            setTextColor(0xFFFFFFFF.toInt())
        }

        apiVersionTextView = TextView(this).apply {
            id = View.generateViewId()
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply { topMargin = dp(4) }
            setTextAppearance(androidx.appcompat.R.style.TextAppearance_AppCompat_Caption)
            setTextColor(0xB0FFFFFF.toInt())
            text = "Vulkan"
        }

        infoTextView = TextView(this).apply {
            id = View.generateViewId()
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply { topMargin = dp(4) }
            text = "大规模球体渲染测试 (Vulkan) - ${String.format("%,d", sphereCount)}个球体"
            setTextAppearance(androidx.appcompat.R.style.TextAppearance_AppCompat_Caption)
            setTextColor(0xB0FFFFFF.toInt())
        }

        val fpsCardInner = LinearLayout(this).apply {
            layoutParams = ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            )
            orientation = LinearLayout.VERTICAL
            addView(fpsTextView)
            addView(apiVersionTextView)
            addView(infoTextView)
        }

        val fpsCardView = MaterialCardView(this).apply {
            id = View.generateViewId()
            layoutParams = FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply {
                gravity = Gravity.TOP
                setMargins(dp(16), dp(16), dp(16), 0)
            }
            setCardBackgroundColor(0x801C1B1F.toInt())
            radius = dp(12).toFloat()
            setContentPadding(dp(16), dp(16), dp(16), dp(16))
            addView(fpsCardInner)
        }

        // Performance result card
        performanceTextView = TextView(this).apply {
            id = View.generateViewId()
            layoutParams = ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            )
            gravity = Gravity.CENTER
            textSize = 18f
            setTextColor(0xFFFFFFFF.toInt())
            text = "性能评级结果将在此显示"
        }

        performanceCardView = MaterialCardView(this).apply {
            id = View.generateViewId()
            layoutParams = FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply {
                gravity = Gravity.BOTTOM
                setMargins(dp(16), 0, dp(16), dp(16))
            }
            visibility = View.GONE
            setCardBackgroundColor(0x8032214A.toInt())
            radius = dp(12).toFloat()
            setContentPadding(dp(16), dp(16), dp(16), dp(16))
            addView(performanceTextView)
        }

        ViewCompat.setOnApplyWindowInsetsListener(fpsCardView) { view, insets ->
            val systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars())
            view.updateLayoutParams<FrameLayout.LayoutParams> {
                topMargin = systemBars.top + dp(16)
                leftMargin = dp(16)
                rightMargin = dp(16)
            }
            insets
        }

        ViewCompat.setOnApplyWindowInsetsListener(performanceCardView) { view, insets ->
            val systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars())
            view.updateLayoutParams<FrameLayout.LayoutParams> {
                bottomMargin = systemBars.bottom + dp(16)
                leftMargin = dp(16)
                rightMargin = dp(16)
            }
            insets
        }

        val root = FrameLayout(this).apply {
            layoutParams = ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
            )
            setBackgroundColor(0xFF1C1B1F.toInt())
            addView(surfaceView)
            addView(fpsCardView)
            addView(performanceCardView)
        }

        setContentView(root)

        startTime = System.currentTimeMillis()
        startFPSCounter()

        // Check Vulkan support
        if (!VulkanTestUtils.isSupported()) {
            Toast.makeText(this, "Vulkan 不可用，无法加载原生库", Toast.LENGTH_LONG).show()
            handler.postDelayed({ finish() }, 2000)
        }
    }

    private fun startRendering() {
        if (isRendering) return
        isRendering = true

        renderThread = Thread {
            val holder = surfaceView?.holder ?: return@Thread
            val surface = holder.surface ?: return@Thread

            if (!VulkanTestUtils.init(surface, surfaceView!!.width, surfaceView!!.height, sphereCount)) {
                runOnUiThread {
                    Toast.makeText(this@VulkanTestActivity, "Vulkan 初始化失败", Toast.LENGTH_LONG).show()
                    finish()
                }
                return@Thread
            }

            vulkanInitialized = true

            runOnUiThread {
                try {
                    val deviceInfo = VulkanTestUtils.getDeviceInfo()
                    apiVersionTextView.text = "${deviceInfo.deviceName} | Vulkan ${deviceInfo.apiVersion}"
                } catch (_: Exception) {}
            }

            startPerformanceTest()

            while (isRendering && surfaceReady) {
                VulkanTestUtils.drawFrame()
            }

            VulkanTestUtils.cleanup()
        }.apply {
            name = "VulkanRenderThread"
            start()
        }
    }

    private fun stopRendering() {
        isRendering = false
        renderThread?.join(2000)
        renderThread = null
    }

    private fun startPerformanceTest() {
        isTesting = true
        testStartTime = System.currentTimeMillis()
        testFrameCount = 0

        handler.postDelayed({
            endPerformanceTest()
        }, testDuration)
    }

    private fun endPerformanceTest() {
        isTesting = false
        val testTime = System.currentTimeMillis() - testStartTime
        val avgFPS = VulkanTestUtils.getLatestFPS()

        val performanceRating = getPerformanceRating(avgFPS)
        val deviceInfo = try { VulkanTestUtils.getDeviceInfo() } catch (_: Exception) { null }

        val sb = StringBuilder()
        sb.appendLine("测试结束!")
        sb.appendLine("平均FPS: %.1f".format(avgFPS))
        sb.appendLine("性能评级: $performanceRating")
        if (deviceInfo != null) {
            sb.appendLine("GPU: ${deviceInfo.deviceName}")
            sb.appendLine("Vulkan API: ${deviceInfo.apiVersion}")
        }

        performanceTextView.text = sb.toString()
        performanceCardView.visibility = View.VISIBLE

        val fadeIn = AlphaAnimation(0.0f, 1.0f).apply {
            duration = 1500
            fillAfter = true
        }
        performanceCardView.startAnimation(fadeIn)
    }

    private fun getPerformanceRating(fps: Float): String {
        return when {
            fps >= 60 -> "卓越 (Outstanding)"
            fps >= 45 -> "优秀 (Excellent)"
            fps >= 30 -> "良好 (Good)"
            fps >= 20 -> "中等 (Average)"
            fps >= 10 -> "一般 (Below Average)"
            else -> "较差 (Poor)"
        }
    }

    private fun startFPSCounter() {
        lastTime = System.currentTimeMillis()
        val fpsRunnable = object : Runnable {
            override fun run() {
                val currentTime = System.currentTimeMillis()
                val fps = VulkanTestUtils.getFPS()
                val elapsed = (currentTime - startTime) / 1000
                runOnUiThread {
                    fpsTextView.text = String.format(
                        "FPS: %.1f | Time: %ds", fps, elapsed
                    )
                }
                lastTime = currentTime
                handler.postDelayed(this, 1000)
            }
        }
        handler.postDelayed(fpsRunnable, 1000)
    }

    override fun onPause() {
        super.onPause()
        stopRendering()
        handler.removeCallbacksAndMessages(null)
    }

    override fun onResume() {
        super.onResume()
        if (surfaceReady && !isRendering) {
            startRendering()
        }
    }

    private fun dp(value: Int): Int = (value * resources.displayMetrics.density).toInt()
}
