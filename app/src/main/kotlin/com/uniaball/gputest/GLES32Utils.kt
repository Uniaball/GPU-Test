package com.uniaball.gputest

import android.opengl.GLES32

object GLES32Utils {
    fun getGLVersion(): String = GLES32.glGetString(GLES32.GL_VERSION)

    fun getGPUInfo(): String = GLES32.glGetString(GLES32.GL_RENDERER)
}
