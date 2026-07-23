package com.uniaball.gputest

import android.content.Context
import android.content.SharedPreferences
import android.os.Bundle
import android.view.ViewGroup
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import com.google.android.material.slider.Slider

class SettingsActivity : AppCompatActivity() {

    companion object {
        private const val PREF_NAME = "Settings"
        private const val KEY_BALL_COUNT = "ballCount"
        private const val MIN_BALL_COUNT = 10000
        private const val DEFAULT_BALL_COUNT = 100000
        private const val MAX_BALL_COUNT = 500000

        fun getSphereCount(context: Context): Int {
            val prefs = context.getSharedPreferences(PREF_NAME, Context.MODE_PRIVATE)
            val storedValue = prefs.getInt(KEY_BALL_COUNT, DEFAULT_BALL_COUNT)
            return clampValue(storedValue, MIN_BALL_COUNT, MAX_BALL_COUNT)
        }

        private fun clampValue(value: Int, min: Int, max: Int): Int = minOf(maxOf(value, min), max)
    }

    private lateinit var ballCountValue: TextView
    private lateinit var ballCountSeekBar: Slider
    private var lastValidValue: Int = 0

    override fun onCreate(savedInstanceState: Bundle?) {
        enableEdgeToEdge()
        super.onCreate(savedInstanceState)

        val ballCountValue = TextView(this).apply {
            id = View.generateViewId()
            layoutParams = LinearLayout.LayoutParams(
                0,
                ViewGroup.LayoutParams.WRAP_CONTENT,
                1f
            )
            gravity = android.view.Gravity.END
            textSize = 16f
        }

        val ballCountLabel = TextView(this).apply {
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            )
            text = "球体数量"
            textSize = 16f
        }

        this.ballCountValue = ballCountValue

        val countRow = LinearLayout(this).apply {
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            )
            orientation = LinearLayout.HORIZONTAL
            gravity = android.view.Gravity.CENTER_VERTICAL
            addView(ballCountLabel)
            addView(ballCountValue)
        }

        val rangeMin = TextView(this).apply {
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            )
            text = "10,000"
            textSize = 14f
        }

        val rangeSpacer = android.view.View(this).apply {
            layoutParams = LinearLayout.LayoutParams(
                0,
                ViewGroup.LayoutParams.WRAP_CONTENT,
                1f
            )
        }

        val rangeMax = TextView(this).apply {
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            )
            text = "500,000"
            textSize = 14f
        }

        val rangeRow = LinearLayout(this).apply {
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            )
            orientation = LinearLayout.HORIZONTAL
            addView(rangeMin)
            addView(rangeSpacer)
            addView(rangeMax)
        }

        ballCountSeekBar = Slider(this).apply {
            id = View.generateViewId()
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            )
        }

        val titleText = TextView(this).apply {
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply { bottomMargin = dp(8) }
            text = "GL ES测试设置"
            textSize = 18f
            setTypeface(null, android.graphics.Typeface.BOLD)
        }

        val settingsBlock = LinearLayout(this).apply {
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            )
            orientation = LinearLayout.VERTICAL
            setPadding(dp(16), dp(16), dp(16), dp(16))
            setBackgroundResource(android.R.attr.selectableItemBackground)
            elevation = 1f
            addView(titleText)
            addView(countRow)
            addView(ballCountSeekBar)
            addView(rangeRow)
        }

        val warningText = TextView(this).apply {
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            )
            text = "注意：增加球体数量将显著提高GPU负载"
            setTextColor(getColor(com.google.android.material.R.color.material_error))
            setPadding(dp(16), dp(16), dp(16), dp(16))
            textSize = 14f
        }

        val contentContainer = LinearLayout(this).apply {
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            )
            orientation = LinearLayout.VERTICAL
            addView(settingsBlock)
            addView(warningText)
        }

        val root = ScrollView(this).apply {
            layoutParams = ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
            )
            fitsSystemWindows = true
            setPadding(dp(16), dp(16), dp(16), dp(16))
            addView(contentContainer)
        }

        setContentView(root)

        initSlider()
    }

    private fun initSlider() {
        ballCountSeekBar.valueFrom = MIN_BALL_COUNT.toFloat()
        ballCountSeekBar.valueTo = MAX_BALL_COUNT.toFloat()
        ballCountSeekBar.stepSize = 10000f

        val initialValue = getSphereCount(this)
        ballCountSeekBar.value = initialValue.toFloat()
        updateBallCountText(initialValue)
        lastValidValue = initialValue

        ballCountSeekBar.addOnSliderTouchListener(object : Slider.OnSliderTouchListener {
            override fun onStartTrackingTouch(slider: Slider) {}

            override fun onStopTrackingTouch(slider: Slider) {
                val currentValue = slider.value.toInt()
                if (currentValue > 200000 && lastValidValue <= 200000) {
                    showHighLoadWarningDialog(currentValue)
                } else {
                    lastValidValue = currentValue
                }
            }
        })

        ballCountSeekBar.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                val intValue = value.toInt()
                saveBallCountSetting(intValue)
                updateBallCountText(intValue)
            }
        }
    }

    private fun updateBallCountText(value: Int) {
        ballCountValue.text = String.format("%,d", value)
    }

    private fun showHighLoadWarningDialog(selectedCount: Int) {
        MaterialAlertDialogBuilder(this)
            .setTitle("高负载警告")
            .setMessage(
                String.format(
                    "您选择了 %,d 个球体（约 %,d 万），这可能大幅度增加GPU负载！\n\n" +
                    "建议仅在测试环境下使用此设置。",
                    selectedCount, selectedCount / 10000
                )
            )
            .setPositiveButton("确定") { dialog, _ -> dialog.dismiss() }
            .setNegativeButton("取消") { dialog, _ ->
                resetToDefaultValue()
                dialog.dismiss()
            }
            .show()
    }

    private fun resetToDefaultValue() {
        ballCountSeekBar.value = DEFAULT_BALL_COUNT.toFloat()
        saveBallCountSetting(DEFAULT_BALL_COUNT)
        updateBallCountText(DEFAULT_BALL_COUNT)
        lastValidValue = DEFAULT_BALL_COUNT
    }

    private fun saveBallCountSetting(ballCount: Int) {
        val clampedValue = clampValue(ballCount, MIN_BALL_COUNT, MAX_BALL_COUNT)
        val editor: SharedPreferences.Editor = getSharedPreferences(PREF_NAME, MODE_PRIVATE).edit()
        editor.putInt(KEY_BALL_COUNT, clampedValue)
        editor.apply()
    }

    private fun dp(value: Int): Int = (value * resources.displayMetrics.density).toInt()
}
