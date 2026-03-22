#ifndef AI_VIEW_MODEL_H
#define AI_VIEW_MODEL_H

#include <QObject>
#include "model/app_settings.h"
#include "ai_call/ai_call_base.h"

class StickyImageStore;   // 前向声明，避免循环依赖

// AIViewModel
// -----------
// 负责 AI 图像识别的请求编排，以及 apiKey / selectedModel 的
// Q_PROPERTY 暴露（数据由 AppSettings Model 持久化）。
//
// selectedModel 枚举：
//   0 → 阿里云百炼（QwenAICall）
//   1 → 火山引擎  （SeedreamAiCall）

class AIViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString apiKey      READ apiKey      NOTIFY apiKeyChanged)
    Q_PROPERTY(int selectedModel   READ selectedModel NOTIFY selectedModelChanged)
    Q_PROPERTY(bool isLoading      READ isLoading   NOTIFY isLoadingChanged)

public:
    explicit AIViewModel(AppSettings &settings,
                         StickyImageStore &store,
                         QObject *parent = nullptr);

    QString apiKey()       const;
    int     selectedModel() const;
    bool    isLoading()    const;

    Q_INVOKABLE bool    setApiKey(const QString &key);
    Q_INVOKABLE void    setSelectedModel(int index);

    // 发起 AI 图像编辑请求
    // prompt   : 用户提示词
    // imageUrl : 当前贴图的 image://sticky/... URL（用于读取原图）
    Q_INVOKABLE void sendPrompt(const QString &prompt, const QString &imageUrl);

signals:
    void apiKeyChanged(const QString &apiKey);
    void selectedModelChanged(int index);

    // AI 生成完毕，oldImageUrl 是请求时的原图 URL，newImageUrl 是生成的新图 URL
    // 调用方应检查 oldImageUrl 是否与自己的 imageUrl 一致再替换
    void signalRequestComplete(const QString &oldImageUrl, const QString &newImageUrl);

    // AI 请求失败，返回错误描述
    void signalRequestFailed(const QString &errorMsg);

    void isLoadingChanged(bool loading);

private:
    void rebuildAiCall();
    void setLoading(bool v);

    AppSettings      &m_settings;
    StickyImageStore &m_store;
    AICallBase       *m_aiCall   = nullptr;
    bool              m_loading  = false;
};

#endif // AI_VIEW_MODEL_H
