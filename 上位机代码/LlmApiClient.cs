using System.Collections;
using System;
using UnityEngine;
using UnityEngine.Networking;
using System.Text;

public class LlmApiClient : MonoBehaviour
{
    [Header("API配置")]
    public string apiKey = "87a76f52-2e2d-4c9c-8108-9d8b1973bb5e";
    public string endpoint = "https://ark.cn-beijing.volces.com/api/v3/chat/completions";
    public string model = "doubao-seed-1-6-flash-250715";
    
    [Header("参数")]
    public float temperature = 0.7f;
    public int maxTokens = 200;
    
    public event Action<string> OnResponseUpdate;
    public event Action<string> OnResponseEnd;
    
    private bool _isStreaming;
    private UnityWebRequest _webRequest;
    private string _currentResponse;
    private string _lastProcessedData;
    
    void Update()
    {
        if (_isStreaming && _webRequest != null)
        {
            ProcessStreamingResponse();
        }
    }
    
    public void SendChatMessage(string userMessage)
    {
        if (string.IsNullOrEmpty(apiKey))
        {
            Debug.LogError("API密钥未设置！");
            return;
        }
        
        if (_isStreaming)
        {
            Debug.LogWarning("正在处理请求，请稍候");
            return;
        }
        
        StartCoroutine(SendRequest(userMessage));
    }
    
    private IEnumerator SendRequest(string userMessage)
    {
        _isStreaming = true;
        _currentResponse = "";
        _lastProcessedData = "";
        
        // 构建干净的JSON
        string jsonData = BuildSimpleJson(userMessage);
        Debug.Log("发送的JSON: " + jsonData);
        
        byte[] bodyRaw = Encoding.UTF8.GetBytes(jsonData);
        
        _webRequest = new UnityWebRequest(endpoint, "POST");
        _webRequest.uploadHandler = new UploadHandlerRaw(bodyRaw);
        _webRequest.downloadHandler = new DownloadHandlerBuffer();
        _webRequest.SetRequestHeader("Content-Type", "application/json");
        _webRequest.SetRequestHeader("Authorization", "Bearer " + apiKey);
        
        _webRequest.SendWebRequest();
        
        while (!_webRequest.isDone)
        {
            ProcessStreamingResponse();
            yield return null;
        }
        
        if (_webRequest.result == UnityWebRequest.Result.Success)
        {
            ProcessStreamingResponse();
            OnResponseEnd?.Invoke(_currentResponse);
            Debug.Log("完成: " + _currentResponse);
        }
        else
        {
            string error = $"错误: {_webRequest.responseCode} - {_webRequest.error}";
            Debug.LogError(error);
            OnResponseEnd?.Invoke($"错误: {error}");
        }
        
        _isStreaming = false;
        _webRequest.Dispose();
        _webRequest = null;
    }
    
    private void ProcessStreamingResponse()
    {
        if (_webRequest == null || _webRequest.downloadHandler == null) return;
        
        string downloadedText = _webRequest.downloadHandler.text;
        if (string.IsNullOrEmpty(downloadedText)) return;
        
        // 只处理新增的数据
        if (downloadedText.Length <= _lastProcessedData.Length)
        {
            return;
        }
        
        string newData = downloadedText.Substring(_lastProcessedData.Length);
        _lastProcessedData = downloadedText;
        
        // 解析新增的流式数据
        string[] lines = newData.Split(new[] { "\r\n", "\r", "\n" }, StringSplitOptions.RemoveEmptyEntries);
        
        foreach (string line in lines)
        {
            string trimmedLine = line.Trim();
            if (trimmedLine.StartsWith("data: "))
            {
                string data = trimmedLine.Substring(6).Trim();
                
                if (data == "[DONE]")
                {
                    return;
                }
                
                if (!string.IsNullOrEmpty(data) && data != "[DONE]")
                {
                    try
                    {
                        // 解析JSON响应
                        ResponseData chunk = JsonUtility.FromJson<ResponseData>(data);
                        if (chunk != null && chunk.choices != null && chunk.choices.Length > 0)
                        {
                            string deltaContent = chunk.choices[0].delta?.content ?? "";
                            if (!string.IsNullOrEmpty(deltaContent))
                            {
                                _currentResponse += deltaContent;
                                OnResponseUpdate?.Invoke(_currentResponse);
                            }
                        }
                    }
                    catch (Exception)
                    {
                        // 静默处理解析错误，可能是部分数据
                    }
                }
            }
        }
    }
    
    private string BuildSimpleJson(string userMessage)
    {
        string systemContent = "你需要模拟生成类似“每日一签”卡片的文字内容，需遵循以下规则构建：\n\n1. 标题与基础信息\n\n • 当用户求签时开头呈现“每日一签” ，作为标题，明确功能场景。\n\n • 包含年、月份 星期信息（如 “2025/7/26 星期六 蛇年乙巳/癸未月”）\n\n2. 签文结构\n • 要有签的类别（如上签、中签、下签 ）及对应宫位，放置在显眼位置（类似示例中右上角布局逻辑 ）。\n\n • 签文内容\n◦ 需包含明确对象（如“白羊座今日运势”“属兔人今日运势”）；\n ◦ 覆盖核心维度：整体运势（用1-5星或“平顺”“上扬”等词描述）\n\n3. 落款 \n  生成今日幸运秘诀：结合用户运势生成简短的文案，可以模仿星巴克的口令（如“一切尽在掌握”“爱不打烊”“万事无忧”等 今日幸运秘诀需要黑体加粗\n\n整体文字简洁、表意清晰，具有哲理性与正念引导另外，不要返回 markdown 格式的文本，并且在每个部分分界插入换行符，生成美观的排版内容";
        string escapedUser = EscapeString(userMessage);
        string escapedSystem = EscapeString(systemContent);
        
        return $"{{\"model\":\"{model}\",\"messages\":[{{\"role\":\"system\",\"content\":\"{escapedSystem}\"}},{{\"role\":\"user\",\"content\":\"{escapedUser}\"}}],\"stream\":true,\"temperature\":{temperature:F2},\"max_tokens\":{maxTokens}}}";
    }
    
    private string EscapeString(string str)
    {
        if (string.IsNullOrEmpty(str)) return "";
        return str.Replace("\\", "\\\\")
                  .Replace("\"", "\\\"")
                  .Replace("\n", "\\n")
                  .Replace("\r", "")
                  .Replace("\t", " ");
    }
    
    void OnDestroy()
    {
        if (_webRequest != null)
        {
            _webRequest.Abort();
            _webRequest.Dispose();
        }
    }
}

[Serializable]
public class ResponseData
{
    public ChoiceData[] choices;
}

[Serializable]
public class ChoiceData
{
    public DeltaData delta;
}

[Serializable]
public class DeltaData
{
    public string content;
}