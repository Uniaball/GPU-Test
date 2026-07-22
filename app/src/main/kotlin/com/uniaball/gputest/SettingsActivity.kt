package com.uniaball.gputest

import android.content.Context
import android.content.SharedPreferences
import android.os.Build
import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import com.google.android.material.slider.Slider
import com.uniaball.gputest.databinding.ActivitySettingsBinding

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

    private lateinit var binding: ActivitySettingsBinding
    private var lastValidValue: Int = 0

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)



        binding = ActivitySettingsBinding.inflate(layoutInflater)
        setContentView(binding.root)

        initSlider()
    }

    private fun initSlider() {
        binding.ballCountSeekBar.valueFrom = MIN_BALL_COUNT.toFloat()
        binding.ballCountSeekBar.valueTo = MAX_BALL_COUNT.toFloat()
        binding.ballCountSeekBar.stepSize = 10000f

        val initialValue = getSphereCount(this)
        binding.ballCountSeekBar.value = initialValue.toFloat()
        updateBallCountText(initialValue)
        lastValidValue = initialValue

        binding.ballCountSeekBar.addOnSliderTouchListener(object : Slider.OnSliderTouchListener {
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

        binding.ballCountSeekBar.addOnChangeListener { _, value, fromUser ->
            if (fromUser) {
                val intValue = value.toInt()
                saveBallCountSetting(intValue)
                updateBallCountText(intValue)
            }
        }
    }

    private fun updateBallCountText(value: Int) {
        binding.ballCountValue.text = String.format("%,d", value)
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
        binding.ballCountSeekBar.value = DEFAULT_BALL_COUNT.toFloat()
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
}
