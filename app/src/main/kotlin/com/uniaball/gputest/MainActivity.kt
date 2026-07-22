package com.uniaball.gputest

import android.content.Intent
import android.opengl.GLSurfaceView
import android.os.Build
import android.os.Bundle
import android.view.Menu
import android.view.MenuItem
import android.view.ViewGroup
import android.widget.Button
import android.widget.TextView
import android.widget.Toast
import androidx.activity.EdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import com.google.android.material.appbar.MaterialToolbar
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class MainActivity : AppCompatActivity() {

    private lateinit var gpuInfoText: TextView
    private var glSurfaceView: GLSurfaceView? = null
    private lateinit var toolbar: MaterialToolbar

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        EdgeToEdge.enable(this)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            window.setNavigationBarContrastEnforced(false)
        }

        toolbar = findViewById(R.id.toolbar)
        setSupportActionBar(toolbar)

        val glTestBtn = findViewById<Button>(R.id.glTestBtn)
        val vulkanTestBtn = findViewById<Button>(R.id.vulkanTestBtn)
        gpuInfoText = findViewById(R.id.gpuInfoText)

        glTestBtn.setOnClickListener { startGLTest() }
        vulkanTestBtn.setOnClickListener { showVulkanTest() }

        initGLInfoDetector()
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        menuInflater.inflate(R.menu.main_menu, menu)
        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        val id = item.itemId
        if (id == R.id.action_settings) {
            startActivity(Intent(this, SettingsActivity::class.java))
            return true
        }
        return super.onOptionsItemSelected(item)
    }

    private fun initGLInfoDetector() {
        val view = GLSurfaceView(this)
        glSurfaceView = view
        view.setEGLContextClientVersion(3)
        view.setRenderer(object : GLSurfaceView.Renderer {
            override fun onSurfaceCreated(gl: GL10, config: EGLConfig) {
                val gpu = GLES32Utils.getGPUInfo()
                val glVersion = GLES32Utils.getGLVersion()
                val vulkanSupported = VulkanUtils.isSupported(this@MainActivity)

                runOnUiThread {
                    gpuInfoText.text = String.format(
                        "GPU: %s\nOpenGL: %s\nVulkan 1.1: %s",
                        gpu, glVersion, if (vulkanSupported) "支持" else "不支持"
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

    private fun showVulkanTest() {
        val supported = VulkanUtils.isSupportedVulkan12(this)
        val message = if (supported) "支持 Vulkan 1.2" else "不支持 Vulkan 1.2"
        Toast.makeText(this, message, Toast.LENGTH_SHORT).show()
    }
}
