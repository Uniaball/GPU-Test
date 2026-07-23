package com.uniaball.gputest

import android.opengl.GLES32
import android.opengl.GLSurfaceView
import android.opengl.Matrix
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.view.Gravity
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
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.FloatBuffer
import java.nio.ShortBuffer
import java.util.Random
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class GLTestActivity : AppCompatActivity() {
    companion object {
        private const val TAG = "MassiveSphereDemo"
    }

    private var glSurfaceView: GLSurfaceView? = null
    private lateinit var fpsTextView: TextView
    private lateinit var infoTextView: TextView
    private lateinit var performanceTextView: TextView
    private lateinit var performanceCardView: MaterialCardView
    private var frameCount = 0
    private var lastTime: Long = 0
    private var startTime: Long = 0
    private val handler = Handler(Looper.getMainLooper())

    private var isTesting = false
    private var testStartTime: Long = 0
    private var testFrameCount = 0
    private val testDuration = 5000

    private var sphereCount: Int = 0
    private var sphereRenderer: SphereRenderer? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        enableEdgeToEdge()
        super.onCreate(savedInstanceState)

        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)

        val glContainer = FrameLayout(this).apply {
            id = View.generateViewId()
            layoutParams = FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
            )
        }

        // FPS card - top overlay
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

        infoTextView = TextView(this).apply {
            id = View.generateViewId()
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply { topMargin = dp(8) }
            text = "大规模球体渲染测试 - 100,000个球体"
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

        // Performance card - bottom overlay, initially hidden
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
            visibility = android.view.View.GONE
            setCardBackgroundColor(0x8032214A.toInt())
            radius = dp(12).toFloat()
            setContentPadding(dp(16), dp(16), dp(16), dp(16))
            addView(performanceTextView)
        }

        // Adjust overlay card margins to account for system bar insets
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
            addView(glContainer)
            addView(fpsCardView)
            addView(performanceCardView)
        }

        setContentView(root)

        sphereCount = SettingsActivity.getSphereCount(this)

        val testTitle = "大规模球体渲染测试 - ${String.format("%,d", sphereCount)}个球体"
        infoTextView.text = testTitle

        val view = GLSurfaceView(this)
        glSurfaceView = view
        view.setEGLContextClientVersion(3)
        view.setEGLConfigChooser(8, 8, 8, 8, 16, 8)
        sphereRenderer = SphereRenderer()
        view.setRenderer(sphereRenderer)
        glContainer.addView(view)

        startTime = System.currentTimeMillis()
        startFPSCounter()

        startPerformanceTest()
    }

    private fun startPerformanceTest() {
        isTesting = true
        testStartTime = System.currentTimeMillis()
        testFrameCount = 0

        handler.postDelayed({ endPerformanceTest() }, testDuration.toLong())
    }

    private fun endPerformanceTest() {
        isTesting = false
        val testTime = System.currentTimeMillis() - testStartTime
        val avgFPS = (testFrameCount * 1000f) / testTime

        val performanceRating = getPerformanceRating(avgFPS)
        val result = String.format("测试结束!\n平均FPS: %.1f\n性能评级: %s", avgFPS, performanceRating)

        performanceTextView.text = result
        performanceCardView.visibility = android.view.View.VISIBLE

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
                val elapsed = currentTime - lastTime
                if (elapsed > 0) {
                    val fps = (frameCount * 1000f) / elapsed
                    runOnUiThread {
                        fpsTextView.text = String.format(
                            "FPS: %.1f | Time: %ds", fps,
                            (currentTime - startTime) / 1000
                        )
                    }
                }
                frameCount = 0
                lastTime = currentTime

                handler.postDelayed(this, 1000)
            }
        }
        handler.postDelayed(fpsRunnable, 1000)
    }

    override fun onPause() {
        super.onPause()
        glSurfaceView?.onPause()
        glSurfaceView?.queueEvent {
            sphereRenderer?.release()
        }
        handler.removeCallbacksAndMessages(null)
    }

    override fun onResume() {
        super.onResume()
        glSurfaceView?.onResume()
        lastTime = System.currentTimeMillis()
        frameCount = 0
    }

    private fun dp(value: Int): Int = (value * resources.displayMetrics.density).toInt()

    private inner class SphereRenderer : GLSurfaceView.Renderer {
        private var shaderProgram = 0
        private var vertexBuffer = 0
        private var normalBuffer = 0
        private var indexBuffer = 0
        private var vertexCount = 0
        private var indexCount = 0
        private var instanceBuffer = 0
        private var vao = 0

        private val viewMatrix = FloatArray(16)
        private val projectionMatrix = FloatArray(16)
        private val viewProjectionMatrix = FloatArray(16)

        private var startTime: Long = 0

        private var uTimeLoc = 0
        private var uLightPosLoc = 0
        private var uViewProjectionMatrixLoc = 0
        private var uCameraPosLoc = 0

        private val vertexShaderCode = """#version 320 es
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aInstancePos;
layout(location = 3) in vec3 aInstanceParams;

uniform mat4 uViewProjectionMatrix;
uniform float uTime;

out vec3 vPosition;
out vec3 vNormal;

void main() {
    float speed = aInstanceParams.x;
    float rotationSpeed = aInstanceParams.y;
    float offset = aInstanceParams.z;

    float timeOffset = uTime * speed + offset;
    vec3 animatedPos = aInstancePos;
    animatedPos.x += sin(timeOffset) * 5.0;
    animatedPos.z += cos(timeOffset * 0.7) * 5.0;
    animatedPos.y += sin(timeOffset * 1.3) * 2.0;

    float angle = uTime * rotationSpeed + offset * 10.0;
    float sinA = sin(angle);
    float cosA = cos(angle);

    mat3 rotationMatrix = mat3(
        cosA, 0.0, -sinA,
        0.0, 1.0, 0.0,
        sinA, 0.0, cosA
    );

    vec3 rotatedPosition = rotationMatrix * aPosition;
    vec3 rotatedNormal = rotationMatrix * aNormal;

    vec3 worldPos = rotatedPosition + animatedPos;

    gl_Position = uViewProjectionMatrix * vec4(worldPos, 1.0);
    vPosition = worldPos;
    vNormal = rotatedNormal;
}
"""

        private val fragmentShaderCode = """#version 320 es
precision mediump float;
in vec3 vPosition;
in vec3 vNormal;
out vec4 fragColor;

uniform vec3 uLightPos;
uniform vec3 uCameraPos;

vec3 materialColor = vec3(0.8, 0.3, 0.2);

void main() {
    float ambient = 0.1;

    vec3 normal = normalize(vNormal);

    vec3 lightDir = normalize(uLightPos - vPosition);

    vec3 viewDir = normalize(uCameraPos - vPosition);

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);

    float diff = max(dot(normal, lightDir), 0.0);

    vec3 lighting = materialColor * (ambient + diff) + vec3(0.3) * spec;

    float posFactor = vPosition.x * 0.1 + vPosition.y * 0.1 + vPosition.z * 0.1;
    vec3 result = lighting;
    result.r *= 0.8 + sin(posFactor) * 0.2;
    result.g *= 0.8 + cos(posFactor) * 0.2;
    result.b *= 0.8 + sin(posFactor * 1.2) * 0.2;

    fragColor = vec4(result, 1.0);
}
"""

        override fun onSurfaceCreated(unused: GL10, config: EGLConfig) {
            val version = GLES32.glGetString(GLES32.GL_VERSION)
            if (version == null || !version.startsWith("OpenGL ES 3.2")) {
                this@GLTestActivity.runOnUiThread {
                    Toast.makeText(this@GLTestActivity, "需要 OpenGL ES 3.2+ 支持", Toast.LENGTH_LONG).show()
                    finish()
                }
                return
            }

            val vaos = IntArray(1)
            GLES32.glGenVertexArrays(1, vaos, 0)
            vao = vaos[0]
            GLES32.glBindVertexArray(vao)

            val vertexShader = loadShader(GLES32.GL_VERTEX_SHADER, vertexShaderCode)
            val fragmentShader = loadShader(GLES32.GL_FRAGMENT_SHADER, fragmentShaderCode)

            shaderProgram = GLES32.glCreateProgram()
            GLES32.glAttachShader(shaderProgram, vertexShader)
            GLES32.glAttachShader(shaderProgram, fragmentShader)
            GLES32.glLinkProgram(shaderProgram)

            GLES32.glDeleteShader(vertexShader)
            GLES32.glDeleteShader(fragmentShader)

            val linkStatus = IntArray(1)
            GLES32.glGetProgramiv(shaderProgram, GLES32.GL_LINK_STATUS, linkStatus, 0)
            if (linkStatus[0] == 0) {
                Log.e(TAG, "程序链接失败: ${GLES32.glGetProgramInfoLog(shaderProgram)}")
                shaderProgram = 0
                return
            }

            uTimeLoc = GLES32.glGetUniformLocation(shaderProgram, "uTime")
            uLightPosLoc = GLES32.glGetUniformLocation(shaderProgram, "uLightPos")
            uViewProjectionMatrixLoc = GLES32.glGetUniformLocation(shaderProgram, "uViewProjectionMatrix")
            uCameraPosLoc = GLES32.glGetUniformLocation(shaderProgram, "uCameraPos")

            createSphere(0.2f, 16)
            createInstances()
            setupVertexAttributes()

            Matrix.setLookAtM(viewMatrix, 0,
                0f, 20f, 50f,
                0f, 0f, 0f,
                0f, 1f, 0f)

            GLES32.glEnable(GLES32.GL_DEPTH_TEST)
            GLES32.glEnable(GLES32.GL_CULL_FACE)

            GLES32.glBindVertexArray(0)

            startTime = System.currentTimeMillis()
        }

        private fun setupVertexAttributes() {
            GLES32.glUseProgram(shaderProgram)

            GLES32.glBindBuffer(GLES32.GL_ARRAY_BUFFER, vertexBuffer)
            val positionLoc = GLES32.glGetAttribLocation(shaderProgram, "aPosition")
            GLES32.glEnableVertexAttribArray(positionLoc)
            GLES32.glVertexAttribPointer(positionLoc, 3, GLES32.GL_FLOAT, false, 0, 0)

            GLES32.glBindBuffer(GLES32.GL_ARRAY_BUFFER, normalBuffer)
            val normalLoc = GLES32.glGetAttribLocation(shaderProgram, "aNormal")
            GLES32.glEnableVertexAttribArray(normalLoc)
            GLES32.glVertexAttribPointer(normalLoc, 3, GLES32.GL_FLOAT, false, 0, 0)

            GLES32.glBindBuffer(GLES32.GL_ARRAY_BUFFER, instanceBuffer)
            val instancePosLoc = GLES32.glGetAttribLocation(shaderProgram, "aInstancePos")
            GLES32.glEnableVertexAttribArray(instancePosLoc)
            GLES32.glVertexAttribPointer(instancePosLoc, 3, GLES32.GL_FLOAT, false, 6 * 4, 0)
            GLES32.glVertexAttribDivisor(instancePosLoc, 1)

            val instanceParamsLoc = GLES32.glGetAttribLocation(shaderProgram, "aInstanceParams")
            GLES32.glEnableVertexAttribArray(instanceParamsLoc)
            GLES32.glVertexAttribPointer(instanceParamsLoc, 3, GLES32.GL_FLOAT, false, 6 * 4, 3 * 4)
            GLES32.glVertexAttribDivisor(instanceParamsLoc, 1)

            GLES32.glBindBuffer(GLES32.GL_ARRAY_BUFFER, 0)
        }

        override fun onSurfaceChanged(unused: GL10, width: Int, height: Int) {
            if (width == 0 || height == 0) return

            GLES32.glViewport(0, 0, width, height)
            val ratio = width.toFloat() / height
            Matrix.perspectiveM(projectionMatrix, 0, 45.0f, ratio, 0.1f, 1000.0f)

            Matrix.multiplyMM(viewProjectionMatrix, 0, projectionMatrix, 0, viewMatrix, 0)
        }

        override fun onDrawFrame(unused: GL10) {
            if (shaderProgram == 0) return

            GLES32.glClearColor(0.05f, 0.05f, 0.1f, 1.0f)
            GLES32.glClear(GLES32.GL_COLOR_BUFFER_BIT or GLES32.GL_DEPTH_BUFFER_BIT)

            GLES32.glUseProgram(shaderProgram)

            val time = (System.currentTimeMillis() - startTime) / 1000f
            if (uTimeLoc != -1) GLES32.glUniform1f(uTimeLoc, time)

            if (uLightPosLoc != -1) {
                val lightX = Math.sin(time.toDouble()).toFloat() * 50.0f
                val lightY = 50.0f
                val lightZ = Math.cos(time.toDouble()).toFloat() * 50.0f
                GLES32.glUniform3f(uLightPosLoc, lightX, lightY, lightZ)
            }

            if (uCameraPosLoc != -1) {
                val invViewMatrix = FloatArray(16)
                Matrix.invertM(invViewMatrix, 0, viewMatrix, 0)
                val cameraX = invViewMatrix[12]
                val cameraY = invViewMatrix[13]
                val cameraZ = invViewMatrix[14]
                GLES32.glUniform3f(uCameraPosLoc, cameraX, cameraY, cameraZ)
            }

            if (uViewProjectionMatrixLoc != -1) {
                GLES32.glUniformMatrix4fv(uViewProjectionMatrixLoc, 1, false, viewProjectionMatrix, 0)
            }

            GLES32.glBindVertexArray(vao)
            GLES32.glBindBuffer(GLES32.GL_ELEMENT_ARRAY_BUFFER, indexBuffer)

            GLES32.glDrawElementsInstanced(
                GLES32.GL_TRIANGLES,
                indexCount,
                GLES32.GL_UNSIGNED_SHORT,
                0,
                sphereCount
            )

            GLES32.glBindVertexArray(0)
            GLES32.glBindBuffer(GLES32.GL_ELEMENT_ARRAY_BUFFER, 0)

            frameCount++
            if (isTesting) testFrameCount++
        }

        private fun createSphere(radius: Float, segments: Int) {
            val vCount = (segments + 1) * (segments + 1)
            val vertices = ByteBuffer.allocateDirect(vCount * 3 * 4)
                .order(ByteOrder.nativeOrder()).asFloatBuffer()

            val normals = ByteBuffer.allocateDirect(vCount * 3 * 4)
                .order(ByteOrder.nativeOrder()).asFloatBuffer()

            for (i in 0..segments) {
                val lat = Math.PI * i / segments
                for (j in 0..segments) {
                    val lon = 2 * Math.PI * j / segments
                    val x = (Math.sin(lat) * Math.cos(lon)).toFloat()
                    val y = Math.cos(lat).toFloat()
                    val z = (Math.sin(lat) * Math.sin(lon)).toFloat()

                    vertices.put(x * radius)
                    vertices.put(y * radius)
                    vertices.put(z * radius)

                    normals.put(x)
                    normals.put(y)
                    normals.put(z)
                }
            }
            vertices.position(0)
            normals.position(0)

            val iCount = segments * segments * 6
            val indices = ByteBuffer.allocateDirect(iCount * 2)
                .order(ByteOrder.nativeOrder()).asShortBuffer()

            for (i in 0 until segments) {
                for (j in 0 until segments) {
                    val start = i * (segments + 1) + j
                    indices.put(start.toShort())
                    indices.put((start + 1).toShort())
                    indices.put((start + segments + 1).toShort())
                    indices.put((start + segments + 1).toShort())
                    indices.put((start + 1).toShort())
                    indices.put((start + segments + 2).toShort())
                }
            }
            indices.position(0)

            val buffers = IntArray(3)
            GLES32.glGenBuffers(3, buffers, 0)

            vertexBuffer = buffers[0]
            GLES32.glBindBuffer(GLES32.GL_ARRAY_BUFFER, vertexBuffer)
            GLES32.glBufferData(GLES32.GL_ARRAY_BUFFER, vertices.remaining() * 4, vertices, GLES32.GL_STATIC_DRAW)

            normalBuffer = buffers[1]
            GLES32.glBindBuffer(GLES32.GL_ARRAY_BUFFER, normalBuffer)
            GLES32.glBufferData(GLES32.GL_ARRAY_BUFFER, normals.remaining() * 4, normals, GLES32.GL_STATIC_DRAW)

            indexBuffer = buffers[2]
            GLES32.glBindBuffer(GLES32.GL_ELEMENT_ARRAY_BUFFER, indexBuffer)
            GLES32.glBufferData(GLES32.GL_ELEMENT_ARRAY_BUFFER, indices.remaining() * 2, indices, GLES32.GL_STATIC_DRAW)

            this.vertexCount = vCount
            this.indexCount = iCount

            GLES32.glBindBuffer(GLES32.GL_ARRAY_BUFFER, 0)
            GLES32.glBindBuffer(GLES32.GL_ELEMENT_ARRAY_BUFFER, 0)
        }

        private fun createInstances() {
            val instanceData = FloatArray(sphereCount * 6)

            val rand = Random()
            for (i in 0 until sphereCount) {
                val x = (rand.nextFloat() - 0.5f) * 200.0f
                val y = (rand.nextFloat() - 0.5f) * 200.0f
                val z = (rand.nextFloat() - 0.5f) * 200.0f

                val speed = 0.1f + rand.nextFloat() * 0.4f
                val rotationSpeed = 0.5f + rand.nextFloat() * 1.0f
                val offset = rand.nextFloat() * 10.0f

                instanceData[i * 6] = x
                instanceData[i * 6 + 1] = y
                instanceData[i * 6 + 2] = z
                instanceData[i * 6 + 3] = speed
                instanceData[i * 6 + 4] = rotationSpeed
                instanceData[i * 6 + 5] = offset
            }

            val buffers = IntArray(1)
            GLES32.glGenBuffers(1, buffers, 0)
            instanceBuffer = buffers[0]

            val instanceBufferData = ByteBuffer.allocateDirect(instanceData.size * 4)
                .order(ByteOrder.nativeOrder()).asFloatBuffer()
            instanceBufferData.put(instanceData).position(0)

            GLES32.glBindBuffer(GLES32.GL_ARRAY_BUFFER, instanceBuffer)
            GLES32.glBufferData(GLES32.GL_ARRAY_BUFFER, instanceData.size * 4, instanceBufferData, GLES32.GL_STATIC_DRAW)

            GLES32.glBindBuffer(GLES32.GL_ARRAY_BUFFER, 0)
        }

        private fun loadShader(type: Int, shaderCode: String): Int {
            val shader = GLES32.glCreateShader(type)
            GLES32.glShaderSource(shader, shaderCode)
            GLES32.glCompileShader(shader)

            val compiled = IntArray(1)
            GLES32.glGetShaderiv(shader, GLES32.GL_COMPILE_STATUS, compiled, 0)
            if (compiled[0] == 0) {
                val log = GLES32.glGetShaderInfoLog(shader)
                Log.e(TAG, "着色器编译失败: $log")
                GLES32.glDeleteShader(shader)
                return 0
            }
            return shader
        }

        fun release() {
            if (shaderProgram != 0) {
                GLES32.glDeleteProgram(shaderProgram)
                shaderProgram = 0
            }

            val buffersToDelete = intArrayOf(vertexBuffer, normalBuffer, indexBuffer, instanceBuffer)
            GLES32.glDeleteBuffers(buffersToDelete.size, buffersToDelete, 0)

            if (vao != 0) {
                val vaos = intArrayOf(vao)
                GLES32.glDeleteVertexArrays(1, vaos, 0)
                vao = 0
            }
        }
    }
}
