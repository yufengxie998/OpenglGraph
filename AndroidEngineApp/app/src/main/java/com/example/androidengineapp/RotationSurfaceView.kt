package com.example.androidengineapp

import android.content.Context
import android.opengl.GLSurfaceView
import android.util.AttributeSet
import android.util.Log
import android.view.MotionEvent

private const val TAG = "RotationSurfaceView"

class RotationSurfaceView(context: Context, attributeSet: AttributeSet) :
    GLSurfaceView(context, attributeSet) {
    private var preX = 0f
    private var preY  = 0f

    private var curX = 0f
    private var curY = 0f

    override fun onTouchEvent(event: MotionEvent?): Boolean {
        when (event?.action) {
            MotionEvent.ACTION_DOWN -> {
                preX = event.rawX
                preY = event.rawY
                Log.d(TAG, "MotionEvent.ACTION_DOWN, x=${event.rawX}, y=${event.rawY}")
            }

            MotionEvent.ACTION_MOVE -> {
                curX = event.rawX
                curY = event.rawY


                Log.d(TAG, "MotionEvent.ACTION_MOVE, x=${event.rawX}, y=${event.rawY}")
            }

            MotionEvent.ACTION_UP -> {
                Log.d(TAG, "MotionEvent.ACTION_UP, x=${event.rawX}, y=${event.rawY}")
            }

            MotionEvent.ACTION_CANCEL -> {
                Log.d(TAG, "MotionEvent.ACTION_CANCEL, x=${event.rawX}, y=${event.rawY}")
            }
        }
        return true
    }
}