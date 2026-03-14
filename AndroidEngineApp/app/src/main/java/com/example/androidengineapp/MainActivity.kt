package com.example.androidengineapp

import android.opengl.GLSurfaceView
import android.os.Bundle
import android.util.Log
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import android.view.MotionEvent
import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class MainActivity : AppCompatActivity() {
    private val TAG = "MainActivity"
    private lateinit var glSurfaceView: GLSurfaceView
    private lateinit var textView: TextView
    private var lastTime: Long = 0
    private var engineInitialized = false

        // 多点触摸相关
    private var lastPinchDistance = -1f
    private var lastRotateAngle = 0f
    private val touchSensitivity = 0.2f // 可以调整


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // 初始化视图
        glSurfaceView = findViewById(R.id.glSurfaceView)
        textView = findViewById(R.id.textView)

        // 设置 OpenGL ES 版本为 3.0
        glSurfaceView.setEGLContextClientVersion(3)

        glSurfaceView.setOnTouchListener { _, event ->
            handleTouchEvent(event)
            true // 返回true表示消费事件
        }
        // 设置渲染器
        glSurfaceView.setRenderer(object : GLSurfaceView.Renderer {
            override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
                Log.d(TAG, "onSurfaceCreated called")

                lastTime = System.nanoTime()

            }

            override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
                // Surface 大小改变时调用
                // 可以在这里更新视口
                android.util.Log.d(TAG, "onSurfaceChanged: $width x $height")

                // 【关键】在这里初始化引擎，而不是在 onSurfaceCreated 中
                if (!engineInitialized) {
                    engineInitialized = true

                 // 关键步骤：将assets中的文件复制到内部存储
                 copyAssetsToInternalStorage()
                
                // 获取内部存储路径
                 val internalPath = filesDir.absolutePath
                 Log.d(TAG, "Internal storage path: $internalPath")

                    // 验证所有需要的文件是否存在
                    verifyFiles(internalPath)

                    initEngine(width, height,internalPath)

                    // 启用测试三角形

                    setTouchSensitivity(touchSensitivity)
                    runOnUiThread {
                        textView.text = stringFromJNI()
                    }
                }
            }

            override fun onDrawFrame(gl: GL10?) {
                if (!engineInitialized) return

                val currentTime = System.nanoTime()
                val deltaTime = (currentTime - lastTime) / 1000000000.0f
                lastTime = currentTime

                drawFrame(deltaTime)

                // // 更新 UI 显示状态
                // runOnUiThread {
                //     textView.text = stringFromJNI()
                // }
            }
        })

        // 设置为连续渲染模式
        glSurfaceView.renderMode = GLSurfaceView.RENDERMODE_CONTINUOUSLY
    }

        // 处理触摸事件
    private fun handleTouchEvent(event: MotionEvent): Boolean {
        val action = event.actionMasked
        val pointerCount = event.pointerCount
        
        when (action) {
            MotionEvent.ACTION_DOWN,
            MotionEvent.ACTION_POINTER_DOWN,
            MotionEvent.ACTION_MOVE -> {
                when (pointerCount) {
                    1 -> {
                        // 单指：旋转摄像机
                        val x = event.getX()
                        val y = event.getY()
                        handleTouchEvent(action, x, y)
                    }
                    2 -> {
                        // 双指：处理缩放和旋转
                        handleTwoFingerGesture(event)
                    }
                }
            }
            
            MotionEvent.ACTION_UP,
            MotionEvent.ACTION_POINTER_UP,
            MotionEvent.ACTION_CANCEL -> {
                // 手指抬起，重置状态
                handleTouchEvent(action, 0f, 0f)
                lastPinchDistance = -1f
                lastRotateAngle = 0f
            }
        }
        
        return true
    }
    
    // 处理双指手势
    private fun handleTwoFingerGesture(event: MotionEvent) {
        if (event.pointerCount < 2) return
        
        val x0 = event.getX(0)
        val y0 = event.getY(0)
        val x1 = event.getX(1)
        val y1 = event.getY(1)
        
        // 计算两点距离（用于缩放）
        val dx = x1 - x0
        val dy = y1 - y0
        val distance = Math.sqrt((dx * dx + dy * dy).toDouble()).toFloat()
        
        // 计算角度（用于旋转）
        val angle = Math.atan2(dy.toDouble(), dx.toDouble()).toFloat()
        
        if (lastPinchDistance > 0) {
            // 缩放因子
            val scaleFactor = distance / lastPinchDistance
            if (Math.abs(scaleFactor - 1.0f) > 0.01f) {
                handlePinchEvent(scaleFactor)
            }
            
            // 旋转角度（可选）
            if (lastRotateAngle != 0f) {
                val rotateDelta = angle - lastRotateAngle
                if (Math.abs(rotateDelta) > 0.01f) {
                    handleRotateEvent(rotateDelta)
                }
            }
        }
        
        lastPinchDistance = distance
        lastRotateAngle = angle
        
        // 也传递移动事件给单指处理（可选）
        val x = (x0 + x1) / 2
        val y = (y0 + y1) / 2
        handleTouchEvent(event.actionMasked, x, y)
    }

    private fun copyAssetsToInternalStorage() {
        // 列出所有需要复制的文件
        val filesToCopy = listOf(
            "cube2.glb",
            "2.png",
            "model_loading_es.vs",   // 注意文件名要匹配
            "model_loading_es.fs",     // 注意文件名要匹配
            "pbr.vert",
            "pbr.frag",
            "skybox/right.jpg",  // 右
            "skybox/left.jpg",   // 左
            "skybox/top.jpg",    // 上
            "skybox/bottom.jpg", // 下
            "skybox/front.jpg",  // 前
            "skybox/back.jpg"    // 后
        )

        for (filename in filesToCopy) {
            try {
                // 打开assets中的文件
                val inputStream = assets.open(filename)
                val outputFile = File(filesDir, filename)

                val parentDir = outputFile.parentFile
                if (parentDir != null && !parentDir.exists()) {
                    val created = parentDir.mkdirs()
                    if (!created) {
                        Log.e(TAG, "Failed to create directory: ${parentDir.absolutePath}")
                        continue
                    }
                    Log.d(TAG, "Created directory: ${parentDir.absolutePath}")
                }
                // 检查文件是否存在且大小相同
                if (!outputFile.exists() || outputFile.length() != inputStream.available().toLong()) {
                    Log.d(TAG, "Copying $filename to internal storage...")
                    FileOutputStream(outputFile).use { outputStream ->
                        inputStream.copyTo(outputStream)
                    }
                    Log.d(TAG, "Successfully copied $filename, size: ${outputFile.length()} bytes")
                } else {
                    Log.d(TAG, "$filename already exists in internal storage, size: ${outputFile.length()} bytes")
                }
                inputStream.close()
            } catch (e: IOException) {
                Log.e(TAG, "Failed to copy $filename from assets", e)
            }
        }
    }

    private fun verifyFiles(internalPath: String) {
        val requiredFiles = listOf(
            "cube2.glb",
            "2.png",
            "model_loading_es.vs",
            "model_loading_es.fs",
            "pbr.vert",
            "pbr.frag",
            "skybox/right.jpg",  // 右
            "skybox/left.jpg",   // 左
            "skybox/top.jpg",    // 上
            "skybox/bottom.jpg", // 下
            "skybox/front.jpg",  // 前
            "skybox/back.jpg"    // 后
        )

        Log.d(TAG, "=== Verifying files in $internalPath ===")
        for (filename in requiredFiles) {
            val file = File(filesDir, filename)
            if (file.exists()) {
                Log.d(TAG, "✓ $filename exists, size: ${file.length()} bytes")
            } else {
                Log.e(TAG, "✗ $filename does NOT exist!")
            }
        }

        // 特别检查着色器文件的内容（可选）
        try {
            val fsFile = File(filesDir, "model_loading_es.fs")
            if (fsFile.exists()) {
                fsFile.inputStream().reader().use { reader ->
                    val firstLine = reader.readText().substring(0, Math.min(50, fsFile.length().toInt()))
                    Log.d(TAG, "FS file first line: $firstLine")
                }
            }
        } catch (e: Exception) {
            Log.e(TAG, "Failed to read shader file", e)
        }
    }

    override fun onResume() {
        super.onResume()
        glSurfaceView.onResume()
        nativeOnResume();
    }

    override fun onPause() {
        super.onPause()
        glSurfaceView.onPause()
        nativeOnPause();
    }

    override fun onDestroy() {
        super.onDestroy()
        // 只在 Activity 真正销毁时关闭引擎
        if (engineInitialized) {
            shutdownEngine()
            engineInitialized = false
        }
    }

    // JNI 函数声明
    external fun initEngine(width: Int, height: Int, internalStoragePath: String)
    external fun enableTestTriangle(enable: Boolean)
    external fun drawFrame(deltaTime: Float)  // 【新增】每帧渲染函数
    external fun shutdownEngine()
    external fun nativeOnPause()
    external fun nativeOnResume()
    external fun stringFromJNI(): String

    external fun handleTouchEvent(action: Int, x: Float, y: Float)
    external fun handlePinchEvent(scaleFactor: Float)
    external fun handleKeyEvent(keyCode: Int, action: Int)
    external fun setTouchSensitivity(sensitivity: Float)
    external fun handleRotateEvent(angle: Float)

    companion object {
        init {
            System.loadLibrary("androidengineapp")
        }
    }
}