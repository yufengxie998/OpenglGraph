/**
 * @file Transform.h
 * @brief 变换和视锥体剔除相关类的头文件
 * 
 * 该文件定义了Transform类以及视锥体剔除相关的结构。
 * Transform类管理对象的局部和全局变换；
 * FrustumMath命名空间包含视锥体、包围体及碰撞检测功能。
 * 
 * @author 谢兴国   1452492081@qq.com
 * @version 1.0
 * @date 2025-10-12
 * 
 * @note 支持层级变换和视锥体剔除功能
 */

 #ifndef _H_TRANSFORM_H
 #define _H_TRANSFORM_H
 
 #include <glm/glm.hpp>
 #include <list>
 #include <array>
 #include <memory>
 
 #include "model.h"
 #include "camera.h"
 
 /**
  * @class Transform
  * @brief 变换类，管理对象的局部和全局变换
  * 
  * @details 提供位置、旋转、缩放的局部空间设置和全局空间计算，
  *          支持层级变换，通过脏标志优化矩阵计算。
  */
 class Transform
 {
 protected:
     //Local space information
     glm::vec3 m_pos = { 0.0f, 0.0f, 0.0f };        ///< 局部位置
     glm::vec3 m_eulerRot = { 0.0f, 0.0f, 0.0f };   ///< 欧拉旋转角（角度制）
     glm::vec3 m_scale = { 1.0f, 1.0f, 1.0f };      ///< 局部缩放
 
     //Global space information concatenate in matrix
     glm::mat4 m_modelMatrix = glm::mat4(1.0f);     ///< 全局模型矩阵
 
     //Dirty flag
     bool m_isDirty = true;                          ///< 脏标志，表示需要重新计算矩阵
 
 protected:
     /**
      * @brief 获取局部模型矩阵
      * @return glm::mat4 局部变换矩阵
      */
     glm::mat4 getLocalModelMatrix();
 
 public:
     /**
      * @brief 计算模型矩阵（无父节点）
      */
     void computeModelMatrix();
 
     /**
      * @brief 计算模型矩阵（有父节点）
      * @param parentGlobalModelMatrix 父节点的全局模型矩阵
      */
     void computeModelMatrix(const glm::mat4& parentGlobalModelMatrix);
 
     /**
      * @brief 设置局部位置
      * @param newPosition 新的局部位置
      */
     void setLocalPosition(const glm::vec3& newPosition);
 
     /**
      * @brief 设置局部旋转（欧拉角）
      * @param newRotation 新的旋转欧拉角
      */
     void setLocalRotation(const glm::vec3& newRotation);
 
     /**
      * @brief 设置局部旋转（轴角）
      * @param angle 旋转角度
      * @param axis 旋转轴
      */
     void setLocalRotationAxis(float angle, const glm::vec3& axis);
 
     /**
      * @brief 设置局部缩放
      * @param newScale 新的缩放值
      */
     void setLocalScale(const glm::vec3& newScale);
 
     /**
      * @brief 获取全局位置
      * @return glm::vec3 世界空间中的位置
      */
     glm::vec3 getGlobalPosition() const;
 
     /**
      * @brief 获取局部位置
      * @return const glm::vec3& 局部位置引用
      */
     const glm::vec3& getLocalPosition() const;
 
     /**
      * @brief 获取局部旋转
      * @return const glm::vec3& 局部旋转欧拉角引用
      */
     const glm::vec3& getLocalRotation() const;
 
     /**
      * @brief 获取局部缩放
      * @return const glm::vec3& 局部缩放引用
      */
     const glm::vec3& getLocalScale() const;
 
     /**
      * @brief 获取模型矩阵
      * @return const glm::mat4& 模型矩阵引用
      */
     const glm::mat4& getModelMatrix() const;
 
     /**
      * @brief 获取右方向向量
      * @return glm::vec3 模型矩阵的X轴方向
      */
     glm::vec3 getRight() const;
 
     /**
      * @brief 获取上方向向量
      * @return glm::vec3 模型矩阵的Y轴方向
      */
     glm::vec3 getUp() const;
 
     /**
      * @brief 获取后方向向量
      * @return glm::vec3 模型矩阵的Z轴方向
      */
     glm::vec3 getBackward() const;
 
     /**
      * @brief 获取前方向向量
      * @return glm::vec3 模型矩阵的-Z轴方向
      */
     glm::vec3 getForward() const;
 
     /**
      * @brief 获取全局缩放
      * @return glm::vec3 各轴的实际缩放值
      */
     glm::vec3 getGlobalScale() const;
 
     /**
      * @brief 检查变换是否已脏
      * @return true 需要重新计算矩阵，false 矩阵是最新的
      */
     bool isDirty() const;
 };
 
 /**
  * @namespace FrustumMath
  * @brief 视锥体数学命名空间，包含视锥体、包围体和碰撞检测功能
  */
 namespace FrustumMath { 
 
 /**
  * @struct Plane
  * @brief 平面结构，用于表示视锥体的各个面
  */
 struct Plane
 {
     glm::vec3 normal = { 0.f, 1.f, 0.f }; ///< 平面法线（单位向量）
     float     distance = 0.f;              ///< 到原点的距离
 
     Plane() = default;
     /**
      * @brief 构造函数
      * @param p1 平面上一点
      * @param norm 平面法线
      */
     Plane(const glm::vec3& p1, const glm::vec3& norm);
 
     /**
      * @brief 获取点到平面的有符号距离
      * @param point 空间点
      * @return float 有符号距离
      */
     float getSignedDistanceToPlane(const glm::vec3& point) const;
 };
 
 /**
  * @struct Frustum
  * @brief 视锥体结构，包含六个面
  */
 struct Frustum
 {
     Plane topFace;     ///< 上面
     Plane bottomFace;  ///< 下面
     Plane rightFace;   ///< 右面
     Plane leftFace;    ///< 左面
     Plane farFace;     ///< 远平面
     Plane nearFace;    ///< 近平面
 };
 
 /**
  * @struct BoundingVolume
  * @brief 包围体基类
  */
 struct BoundingVolume
 {
     virtual ~BoundingVolume() = default;
 
     /**
      * @brief 判断包围体是否在视锥体内
      * @param camFrustum 摄像机视锥体
      * @param transform 物体的变换
      * @return true 在视锥体内，false 在视锥体外
      */
     virtual bool isOnFrustum(const Frustum& camFrustum, const Transform& transform) const = 0;
 
     /**
      * @brief 判断包围体是否在平面前方或与平面相交
      * @param plane 测试平面
      * @return true 在平面前方或相交，false 在平面后方
      */
     virtual bool isOnOrForwardPlane(const Plane& plane) const = 0;
 
     /**
      * @brief 判断包围体是否在视锥体内（不考虑变换）
      * @param camFrustum 摄像机视锥体
      * @return true 在视锥体内，false 在视锥体外
      */
     bool isOnFrustum(const Frustum& camFrustum) const;
 };
 
 /**
  * @struct Sphere
  * @brief 包围球结构
  */
 struct Sphere : public BoundingVolume
 {
     glm::vec3 center{ 0.f, 0.f, 0.f }; ///< 球心
     float radius{ 0.f };                ///< 半径
 
     /**
      * @brief 构造函数
      * @param inCenter 球心
      * @param inRadius 半径
      */
     Sphere(const glm::vec3& inCenter, float inRadius);
 
     bool isOnOrForwardPlane(const Plane& plane) const override;
     bool isOnFrustum(const Frustum& camFrustum, const Transform& transform) const override;
 };
 
 /**
  * @struct SquareAABB
  * @brief 正方形轴向包围盒（各向同性）
  */
 struct SquareAABB : public BoundingVolume
 {
     glm::vec3 center{ 0.f, 0.f, 0.f }; ///< 中心点
     float extent{ 0.f };                ///< 半边长
 
     /**
      * @brief 构造函数
      * @param inCenter 中心点
      * @param inExtent 半边长
      */
     SquareAABB(const glm::vec3& inCenter, float inExtent);
 
     bool isOnOrForwardPlane(const Plane& plane) const override;
     bool isOnFrustum(const Frustum& camFrustum, const Transform& transform) const override;
 };
 
 /**
  * @struct AABB
  * @brief 轴向包围盒（各向异性）
  */
 struct AABB : public BoundingVolume
 {
     glm::vec3 center{ 0.f, 0.f, 0.f };  ///< 中心点
     glm::vec3 extents{ 0.f, 0.f, 0.f }; ///< 各轴半长
 
     /**
      * @brief 构造函数（最小点和最大点）
      * @param min 最小点
      * @param max 最大点
      */
     AABB(const glm::vec3& min, const glm::vec3& max);
 
     /**
      * @brief 构造函数（中心和半长）
      * @param inCenter 中心点
      * @param iI X轴半长
      * @param iJ Y轴半长
      * @param iK Z轴半长
      */
     AABB(const glm::vec3& inCenter, float iI, float iJ, float iK);
 
     /**
      * @brief 获取包围盒的8个顶点
      * @return std::array<glm::vec3, 8> 顶点数组
      */
     std::array<glm::vec3, 8> getVertice() const;
 
     bool isOnOrForwardPlane(const Plane& plane) const override;
     bool isOnFrustum(const Frustum& camFrustum, const Transform& transform) const override;
 };
 
 /**
  * @brief 从摄像机创建视锥体
  * @param cam 摄像机对象
  * @param aspect 宽高比
  * @param fovY 垂直视野角度
  * @param zNear 近平面距离
  * @param zFar 远平面距离
  * @return Frustum 视锥体结构
  */
 Frustum createFrustumFromCamera(const Camera& cam, float aspect, float fovY, float zNear, float zFar);
 
 /**
  * @brief 从模型生成AABB包围盒
  * @param model 模型对象
  * @return AABB 轴向包围盒
  */
 AABB generateAABB(const Model& model);
 
 /**
  * @brief 从模型生成包围球
  * @param model 模型对象
  * @return Sphere 包围球
  */
 Sphere generateSphereBV(const Model& model);
 
 } // namespace FrustumMath
 
 #endif // _H_TRANSFORM_H