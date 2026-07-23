package com.uniaball.gputest

import android.content.Intent
import android.content.pm.PackageManager
import android.opengl.GLSurfaceView
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.text.Editable
import android.text.TextWatcher
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.view.inputmethod.EditorInfo
import android.widget.EditText
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.updatePadding
import com.google.android.material.appbar.MaterialToolbar
import com.google.android.material.button.MaterialButton
import com.google.android.material.card.MaterialCardView
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class MainActivity : AppCompatActivity() {

    private lateinit var gpuInfoText: TextView
    private var glSurfaceView: GLSurfaceView? = null
    private val handler = Handler(Looper.getMainLooper())

    override fun onCreate(savedInstanceState: Bundle?) {
        enableEdgeToEdge()
        super.onCreate(savedInstanceState)

        // Toolbar
        val toolbar = MaterialToolbar(this).apply {
            id = View.generateViewId()
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            )
            title = "GPU Test"
            inflateMenu(R.menu.main_menu)
            setOnMenuItemClickListener { item ->
                if (item.itemId == R.id.action_settings) {
                    startActivity(Intent(this@MainActivity, SettingsActivity::class.java))
                    true
                } else false
            }
        }

        // Info card
        gpuInfoText = TextView(this).apply {
            id = View.generateViewId()
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            )
            text = getString(R.string.loading)
            textSize = 14f
            setLineSpacing(dp(4).toFloat(), 1f)
        }

        val cardInner = LinearLayout(this).apply {
            layoutParams = ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            )
            orientation = LinearLayout.VERTICAL
            setPadding(dp(16), dp(16), dp(16), dp(16))
            addView(gpuInfoText)
        }

        val infoCard = MaterialCardView(this).apply {
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            )
            radius = dp(12).toFloat()
            strokeWidth = 1
            addView(cardInner)
        }

        // OpenGL ES test button (outlined)
        val glTestBtn = MaterialButton(this, null, com.google.android.material.R.attr.materialButtonOutlinedStyle).apply {
            id = View.generateViewId()
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply { topMargin = dp(24) }
            text = getString(R.string.gl_test_btn)
            icon = getDrawable(R.drawable.ic_opengl)
            setOnClickListener { startGLTest() }
        }

        // Vulkan test button (outlined)
        val vulkanTestBtn = MaterialButton(this, null, com.google.android.material.R.attr.materialButtonOutlinedStyle).apply {
            id = View.generateViewId()
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply { topMargin = dp(16) }
            text = getString(R.string.vulkan_test_btn)
            icon = getDrawable(R.drawable.ic_vulkan)
            setOnClickListener { startVulkanTest() }
        }

        // Vulkan detect button (outlined)
        val vulkanDetectBtn = MaterialButton(this, null, com.google.android.material.R.attr.materialButtonOutlinedStyle).apply {
            id = View.generateViewId()
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply { topMargin = dp(16) }
            text = getString(R.string.vulkan_detect_btn)
            icon = getDrawable(R.drawable.ic_timer)
            setOnClickListener { showVulkanDetail() }
        }

        // Footer
        val footer = TextView(this).apply {
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply {
                topMargin = dp(32)
                gravity = Gravity.CENTER_HORIZONTAL
            }
            text = getString(R.string.version_info)
            textSize = 12f
        }

        val contentLayout = LinearLayout(this).apply {
            layoutParams = ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            )
            orientation = LinearLayout.VERTICAL
            setPadding(dp(24), dp(16), dp(24), dp(24))
            addView(infoCard)
            addView(glTestBtn)
            addView(vulkanTestBtn)
            addView(vulkanDetectBtn)
            addView(footer)
        }

        val root = LinearLayout(this).apply {
            layoutParams = ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
            )
            orientation = LinearLayout.VERTICAL
            addView(toolbar)
            addView(contentLayout)
        }

        // Apply window insets to avoid status bar overlap
        ViewCompat.setOnApplyWindowInsetsListener(root) { view, insets ->
            val systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars())
            view.updatePadding(top = systemBars.top)
            insets
        }

        setContentView(root)

        initGLInfoDetector()
    }

    private fun initGLInfoDetector() {
        val view = GLSurfaceView(this)
        glSurfaceView = view
        view.setEGLContextClientVersion(3)
        view.setRenderer(object : GLSurfaceView.Renderer {
            override fun onSurfaceCreated(gl: GL10, config: EGLConfig) {
                val gpu = GLES32Utils.getGPUInfo()
                val glVersion = GLES32Utils.getGLVersion()

                runOnUiThread {
                    gpuInfoText.text = String.format(
                        "GPU: %s\nOpenGL: %s",
                        gpu, glVersion
                    )

                    if (view.parent != null) {
                        (view.parent as ViewGroup).removeView(view)
                    }
                }
            }

            override fun onSurfaceChanged(gl: GL10, width: Int, height: Int) {}
            override fun onDrawFrame(gl: GL10) {}
        })

        addContentView(view, ViewGroup.LayoutParams(1, 1))
    }

    override fun onResume() {
        super.onResume()
        glSurfaceView?.onResume()
    }

    override fun onPause() {
        super.onPause()
        glSurfaceView?.onPause()
    }

    private fun startGLTest() {
        startActivity(Intent(this, GLTestActivity::class.java))
    }

    private fun startVulkanTest() {
        startActivity(Intent(this, VulkanTestActivity::class.java))
    }

    private fun showVulkanDetail() {
        val loadingDialog = MaterialAlertDialogBuilder(this)
            .setTitle("正在检测 Vulkan...")
            .setMessage("请稍候...")
            .setCancelable(false)
            .create()

        try {
            loadingDialog.show()
        } catch (e: Exception) {
            return
        }

        Thread {
            try {
                val basicSupport = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
                    packageManager.hasSystemFeature(PackageManager.FEATURE_VULKAN_HARDWARE_LEVEL)
                } else false

                val basicSupport12 = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                    packageManager.hasSystemFeature(PackageManager.FEATURE_VULKAN_HARDWARE_LEVEL, 2)
                } else false

                var vulkanInfo: VulkanUtils.VulkanInfo? = null
                var detailText: String
                try {
                    vulkanInfo = VulkanUtils.getVulkanInfo()
                    detailText = VulkanUtils.buildDetailText(vulkanInfo)
                } catch (e: Exception) {
                    val sb = StringBuilder()
                    sb.appendLine("Vulkan 基础检测结果:")
                    sb.appendLine()
                    sb.appendLine("Vulkan 1.1 支持: ${if (basicSupport) "支持" else "不支持"}")
                    sb.appendLine("Vulkan 1.2 支持: ${if (basicSupport12) "支持" else "不支持"}")
                    if (!basicSupport) {
                        sb.appendLine()
                        sb.appendLine("详细硬件信息需要通过 JNI 获取，")
                        sb.appendLine("但当前设备原生库加载失败。")
                        sb.appendLine()
                        sb.appendLine("错误: ${e.message}")
                    }
                    detailText = sb.toString()
                }

                val finalInfo = vulkanInfo
                val finalText = detailText

                handler.post {
                    try {
                        loadingDialog.dismiss()
                    } catch (_: Exception) {}

                    try {
                        val searchInput = EditText(this).apply {
                            hint = "搜索 Vulkan 扩展..."
                            setSingleLine()
                            imeOptions = EditorInfo.IME_ACTION_DONE
                            layoutParams = LinearLayout.LayoutParams(
                                ViewGroup.LayoutParams.MATCH_PARENT,
                                ViewGroup.LayoutParams.WRAP_CONTENT
                            ).apply { bottomMargin = dp(12) }
                        }

                        val detailTextView = TextView(this).apply {
                            text = finalText
                            textSize = 13f
                            setLineSpacing(dp(4).toFloat(), 1f)
                        }

                        val scrollView = ScrollView(this).apply {
                            setPadding(dp(32), dp(16), dp(32), dp(16))
                            addView(detailTextView)
                        }

                        // Search listener - only works when JNI info is available
                        if (finalInfo != null) {
                            searchInput.addTextChangedListener(object : TextWatcher {
                                override fun beforeTextChanged(s: CharSequence?, start: Int, count: Int, after: Int) {}
                                override fun onTextChanged(s: CharSequence?, start: Int, before: Int, count: Int) {}
                                override fun afterTextChanged(s: Editable?) {
                                    val filter = s?.toString() ?: ""
                                    detailTextView.text = VulkanUtils.buildDetailText(finalInfo, filter)
                                }
                            })
                        }

                        val dialogLayout = LinearLayout(this).apply {
                            orientation = LinearLayout.VERTICAL
                            setPadding(dp(24), dp(16), dp(24), dp(16))
                            if (finalInfo != null) {
                                addView(searchInput)
                            }
                            addView(scrollView)
                        }

                        val title = if (basicSupport || finalText.contains("\"supported\":true"))
                            "Vulkan 详细信息" else "Vulkan 不可用"

                        MaterialAlertDialogBuilder(this@MainActivity)
                            .setTitle(title)
                            .setView(dialogLayout)
                            .setPositiveButton("关闭", null)
                            .show()
                    } catch (_: Exception) {}
                }
            } catch (_: Exception) {
                handler.post {
                    try { loadingDialog.dismiss() } catch (_: Exception) {}
                }
            }
        }.start()
    }

    private fun dp(value: Int): Int = (value * resources.displayMetrics.density).toInt()
}
