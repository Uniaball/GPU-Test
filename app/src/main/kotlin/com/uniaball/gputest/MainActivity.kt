package com.uniaball.gputest

import android.content.Intent
import android.opengl.GLSurfaceView
import android.os.Bundle
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import com.google.android.material.appbar.MaterialToolbar
import com.google.android.material.button.MaterialButton
import com.google.android.material.card.MaterialCardView
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class MainActivity : AppCompatActivity() {

    private lateinit var gpuInfoText: TextView
    private var glSurfaceView: GLSurfaceView? = null

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

        // OpenGL ES test button
        val glTestBtn = MaterialButton(this).apply {
            id = View.generateViewId()
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply { topMargin = dp(24) }
            text = getString(R.string.gl_test_btn)
            icon = getDrawable(R.drawable.ic_opengl)
            setOnClickListener { startGLTest() }
        }

        // Vulkan detect button
        val vulkanDetectBtn = MaterialButton(this).apply {
            id = View.generateViewId()
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply { topMargin = dp(16) }
            text = getString(R.string.vulkan_detect_btn)
            icon = getDrawable(R.drawable.ic_vulkan)
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
            setPadding(dp(24), dp(24), dp(24), dp(24))
            addView(infoCard)
            addView(glTestBtn)
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

    private fun showVulkanDetail() {
        val dialog = MaterialAlertDialogBuilder(this)
            .setTitle("正在检测 Vulkan...")
            .setMessage("请稍候...")
            .setCancelable(false)
            .create()
        dialog.show()

        Thread {
            val info = VulkanUtils.getVulkanInfo()
            val detailText = VulkanUtils.buildDetailText(info)

            runOnUiThread {
                dialog.dismiss()

                val scrollView = ScrollView(this).apply {
                    setPadding(dp(48), dp(32), dp(48), dp(32))
                    addView(TextView(this@MainActivity).apply {
                        text = detailText
                        textSize = 13f
                        setLineSpacing(dp(4).toFloat(), 1f)
                    })
                }

                val title = if (info.supported) "Vulkan 详细信息" else "Vulkan 不可用"

                MaterialAlertDialogBuilder(this@MainActivity)
                    .setTitle(title)
                    .setView(scrollView)
                    .setPositiveButton("关闭", null)
                    .show()
            }
        }.start()
    }

    private fun dp(value: Int): Int = (value * resources.displayMetrics.density).toInt()
}
