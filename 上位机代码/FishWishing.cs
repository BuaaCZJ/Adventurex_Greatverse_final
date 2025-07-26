using System.Collections;
using UnityEngine;
using UnityEngine.UI;

/// <summary>
/// 控制许愿鱼动画的脚本。
/// 每次该 GameObject 被激活时，都会重新开始动画。
/// </summary>
[RequireComponent(typeof(Image))]
public class FishWishing : MonoBehaviour
{
    [Tooltip("目标旋转速度（度/秒），你可以在 Inspector 中调整此值。")]
    public float rotationSpeed = 720f;
    [Tooltip("包含此动画的父面板，动画结束后会自动禁用。")]
    public GameObject wishingPanel;

    private Image fishImage;
    private Coroutine wishingCoroutine;
    
    // 用于在 OnEnable 中重置动画状态的变量
    private Color initialColor;
    private Quaternion initialRotation;

    void Awake()
    {
        // 获取必要的组件引用
        fishImage = GetComponent<Image>();
        
        // 保存初始状态，以便每次都能重置动画
        initialColor = fishImage.color;
        initialRotation = fishImage.transform.rotation;
    }

    void OnEnable()
    {
        // 每次激活 GameObject 时，重置状态并开始动画
        ResetAndStartWishing();
    }

    void OnDisable()
    {
        // 当 GameObject 被禁用时，停止所有正在运行的协程以防止错误
        if (wishingCoroutine != null)
        {
            StopCoroutine(wishingCoroutine);
            wishingCoroutine = null;
        }
    }

    /// <summary>
    /// 重置动画状态并开始许愿动画。
    /// </summary>
    private void ResetAndStartWishing()
    {
        // 重置为初始状态
        fishImage.color = initialColor;
        fishImage.transform.rotation = initialRotation;
        
        // 确保没有正在运行的旧协程
        if (wishingCoroutine != null)
        {
            StopCoroutine(wishingCoroutine);
        }
        
        // 启动新的动画协程
        wishingCoroutine = StartCoroutine(WishingAnimation());
    }

    private IEnumerator WishingAnimation()
    {
        float elapsedTime = 0f;
        float currentRotationSpeed = 0f;

        // 定义动画各阶段的时间
        const float accelerationDuration = 1f; // 加速时间
        const float totalDuration = 5f;        // 总时间

        while (elapsedTime < totalDuration)
        {
            elapsedTime += Time.deltaTime;

            // --- 旋转逻辑 ---
            if (elapsedTime < accelerationDuration)
            {
                // 在第一秒内平滑加速
                currentRotationSpeed = Mathf.Lerp(0, rotationSpeed, elapsedTime / accelerationDuration);
            }
            else
            {
                // 之后保持恒定速度
                currentRotationSpeed = rotationSpeed;
            }
            
            // 应用旋转
            fishImage.transform.Rotate(0f, 0f, -currentRotationSpeed * Time.deltaTime);

            yield return null; // 等待下一帧
        }

        // --- 动画结束 ---
        OnWishingComplete();
        wishingCoroutine = null;
    }

    /// <summary>
    /// 当许愿动画完成时调用。
    /// </summary>
    private void OnWishingComplete()
    {
        // 播放音效
        SFXManager sfxManager = FindFirstObjectByType<SFXManager>();
        if (sfxManager != null)
        {
            sfxManager.playWishingSound();
        }

        // 增加功德
        GameManager gameManager = FindObjectOfType<GameManager>();
        if (gameManager != null)
        {
            gameManager.gongde += 30;
        }

        // 显示漂浮文字
        GongProgressBar progressBar = FindFirstObjectByType<GongProgressBar>();
        if (progressBar != null)
        {
            progressBar.helpFloatingText("祈愿成功，功德增加");
        }
        
        // 完成后禁用父面板
        if (wishingPanel != null)
        {
            wishingPanel.SetActive(false);
        }
    }
}