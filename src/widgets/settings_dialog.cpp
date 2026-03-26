#include "widgets/settings_dialog.h"

#include "language_manager.h"
#include "viewmodel/ai_view_model.h"
#include "viewmodel/gif_record_view_model.h"
#include "viewmodel/ocr_view_model.h"
#include "viewmodel/storage_view_model.h"
#include "shortcut_key/globalshortcut.h"

#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QStackedWidget>
#include <QStyle>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QWheelEvent>

namespace {

QString loadSettingsDialogStyleSheet()
{
    QFile file(QStringLiteral(":/resource/qss/settings_dialog.qss"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

class HotkeyRecordDialog : public QDialog
{
public:
    HotkeyRecordDialog(const QString &labelText,
                       int actionId,
                       const QString &currentSeq,
                       GlobalShortcut *globalShortcut,
                       QWidget *parent = nullptr)
        : QDialog(parent)
        , m_actionId(actionId)
        , m_globalShortcut(globalShortcut)
        , m_pendingSeq(currentSeq)
    {
        setObjectName(QStringLiteral("hotkeyRecordDialog"));
        setWindowTitle(tr("编辑快捷键"));
        setModal(true);
        setFixedSize(360, 220);
        setStyleSheet(loadSettingsDialogStyleSheet());

        auto *root = new QVBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);

        auto *card = new QFrame(this);
        card->setObjectName(QStringLiteral("hotkeyRecordCard"));
        auto *layout = new QVBoxLayout(card);
        layout->setContentsMargins(24, 24, 24, 24);
        layout->setSpacing(16);

        auto *title = new QLabel(tr("编辑快捷键 · %1").arg(labelText), card);
        title->setObjectName(QStringLiteral("recordTitle"));
        layout->addWidget(title);

        m_keyBox = new QFrame(card);
        m_keyBox->setObjectName(QStringLiteral("recordKeyBox"));
        m_keyBox->setProperty("armed", true);
        auto *keyLayout = new QVBoxLayout(m_keyBox);
        keyLayout->setContentsMargins(0, 0, 0, 0);
        m_keyLabel = new QLabel(m_keyBox);
        m_keyLabel->setObjectName(QStringLiteral("recordKeyText"));
        m_keyLabel->setAlignment(Qt::AlignCenter);
        m_keyLabel->setMinimumHeight(56);
        keyLayout->addWidget(m_keyLabel);
        layout->addWidget(m_keyBox);

        auto *hint = new QLabel(tr("请直接按下新的快捷键组合..."), card);
        hint->setObjectName(QStringLiteral("recordHint"));
        layout->addWidget(hint);

        m_conflictLabel = new QLabel(card);
        m_conflictLabel->setObjectName(QStringLiteral("recordConflict"));
        m_conflictLabel->setWordWrap(true);
        m_conflictLabel->hide();
        layout->addWidget(m_conflictLabel);

        auto *buttons = new QHBoxLayout;
        buttons->addStretch();
        auto *cancel = new QPushButton(tr("取消"), card);
        cancel->setObjectName(QStringLiteral("recordCancel"));
        connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
        buttons->addWidget(cancel);

        m_confirmButton = new QPushButton(tr("确认"), card);
        m_confirmButton->setObjectName(QStringLiteral("recordConfirm"));
        connect(m_confirmButton, &QPushButton::clicked, this, [this]() {
            if (m_pendingSeq.isEmpty() || !m_globalShortcut) {
                return;
            }
            const int conflictId = m_globalShortcut->checkConflict(m_pendingSeq);
            if (conflictId != 0 && conflictId != m_actionId) {
                QString conflictName;
                switch (conflictId) {
                case 1: conflictName = tr("截图"); break;
                case 2: conflictName = tr("截图并保存"); break;
                case 3: conflictName = tr("贴图到桌面"); break;
                case 4: conflictName = tr("显示/隐藏窗口"); break;
                default: break;
                }
                m_conflictLabel->setText(tr("「%1」已被「%2」使用，请换一个。").arg(m_pendingSeq, conflictName));
                m_conflictLabel->show();
                return;
            }
            accept();
        });
        buttons->addWidget(m_confirmButton);
        layout->addLayout(buttons);

        root->addWidget(card);
        refreshUi();
    }

    QString recordedSequence() const
    {
        return m_pendingSeq;
    }

protected:
    void keyPressEvent(QKeyEvent *event) override
    {
        event->accept();
        const QString seq = GlobalShortcut::buildKeySequence(event->key(), int(event->modifiers()));
        if (!seq.isEmpty()) {
            m_pendingSeq = seq;
            m_conflictLabel->hide();
            refreshUi();
        }
    }

private:
    void refreshUi()
    {
        m_keyLabel->setText(m_pendingSeq.isEmpty()
                                ? tr("请按下新的快捷键组合...")
                                : m_pendingSeq);
        m_confirmButton->setEnabled(!m_pendingSeq.isEmpty());
        m_keyBox->style()->unpolish(m_keyBox);
        m_keyBox->style()->polish(m_keyBox);
        m_keyBox->update();
    }

    int m_actionId = 0;
    GlobalShortcut *m_globalShortcut = nullptr;
    QString m_pendingSeq;
    QFrame *m_keyBox = nullptr;
    QLabel *m_keyLabel = nullptr;
    QLabel *m_conflictLabel = nullptr;
    QPushButton *m_confirmButton = nullptr;
};

class SettingsComboBox : public QComboBox
{
public:
    using QComboBox::QComboBox;

protected:
    void wheelEvent(QWheelEvent *event) override
    {
        event->ignore();
    }
};

}

SettingsDialog::SettingsDialog(AIViewModel *aiViewModel,
                               StorageViewModel *storageViewModel,
                               LanguageManager *languageManager,
                               GifRecordViewModel *gifRecordViewModel,
                               OcrViewModel *ocrViewModel,
                               GlobalShortcut *globalShortcut,
                               QWidget *parent)
    : QDialog(parent)
    , m_aiViewModel(aiViewModel)
    , m_storageViewModel(storageViewModel)
    , m_languageManager(languageManager)
    , m_gifRecordViewModel(gifRecordViewModel)
    , m_ocrViewModel(ocrViewModel)
    , m_globalShortcut(globalShortcut)
{
    setObjectName(QStringLiteral("settingsDialog"));
    setWindowTitle(tr("设置"));
    resize(750, 520);
    setModal(false);
    setStyleSheet(loadSettingsDialogStyleSheet());

    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *sidePanel = new QFrame(this);
    sidePanel->setObjectName(QStringLiteral("settingsSidePanel"));
    sidePanel->setFixedWidth(160);
    auto *sideLayout = new QVBoxLayout(sidePanel);
    sideLayout->setContentsMargins(0, 0, 0, 12);
    sideLayout->setSpacing(0);

    auto *brand = new QWidget(sidePanel);
    brand->setFixedHeight(72);
    auto *brandLayout = new QVBoxLayout(brand);
    brandLayout->setContentsMargins(0, 12, 0, 8);
    brandLayout->setSpacing(4);
    auto *badge = new QLabel(QStringLiteral("T"), brand);
    badge->setObjectName(QStringLiteral("settingsBrandBadge"));
    badge->setAlignment(Qt::AlignCenter);
    badge->setFixedSize(36, 36);
    brandLayout->addWidget(badge, 0, Qt::AlignHCenter);
    auto *brandText = new QLabel(QStringLiteral("TZshot"), brand);
    brandText->setObjectName(QStringLiteral("settingsBrandText"));
    brandLayout->addWidget(brandText, 0, Qt::AlignHCenter);
    sideLayout->addWidget(brand);

    auto *sideDivider = new QFrame(sidePanel);
    sideDivider->setObjectName(QStringLiteral("settingsSideDivider"));
    sideDivider->setFixedHeight(1);
    sideLayout->addWidget(sideDivider);
    sideLayout->addSpacing(12);

    m_navList = new QListWidget(sidePanel);
    m_navList->setObjectName(QStringLiteral("settingsNavList"));
    m_navList->setFixedWidth(160);
    m_navList->addItem(tr("通用设置"));
    m_navList->addItem(tr("快捷键设置"));
    m_navList->setCurrentRow(0);
    sideLayout->addWidget(m_navList);
    sideLayout->addStretch();

    auto *version = new QLabel(QStringLiteral("v1.0.0"), sidePanel);
    version->setObjectName(QStringLiteral("settingsBrandText"));
    sideLayout->addWidget(version, 0, Qt::AlignHCenter);

    root->addWidget(sidePanel);

    auto *divider = new QFrame(this);
    divider->setObjectName(QStringLiteral("settingsMainDivider"));
    divider->setFixedWidth(1);
    root->addWidget(divider);

    m_stack = new QStackedWidget(this);
    auto *generalScroll = new QScrollArea(this);
    generalScroll->setWidgetResizable(true);
    generalScroll->setFrameShape(QFrame::NoFrame);
    generalScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    generalScroll->setWidget(buildGeneralPage());
    m_stack->addWidget(generalScroll);
    m_stack->addWidget(buildHotkeyPage());
    root->addWidget(m_stack, 1);

    connect(m_navList, &QListWidget::currentRowChanged, m_stack, &QStackedWidget::setCurrentIndex);
}

void SettingsDialog::showAndActivate()
{
    if (!isVisible()) {
        if (QScreen *screen = QGuiApplication::primaryScreen()) {
            const QRect available = screen->availableGeometry();
            move(available.center() - QPoint(width() / 2, height() / 2));
        }
        show();
    }
    raise();
    activateWindow();
}

QWidget *SettingsDialog::createSectionTitle(const QString &title)
{
    auto *wrap = new QWidget(this);
    auto *layout = new QHBoxLayout(wrap);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto *bar = new QLabel(wrap);
    bar->setObjectName(QStringLiteral("accentBar"));
    bar->setFixedSize(3, 15);
    layout->addWidget(bar);

    auto *label = new QLabel(title, wrap);
    label->setObjectName(QStringLiteral("sectionTitle"));
    layout->addWidget(label);
    layout->addStretch();
    return wrap;
}

QWidget *SettingsDialog::createCard()
{
    auto *card = new QFrame(this);
    card->setObjectName(QStringLiteral("sectionCard"));
    return card;
}

QWidget *SettingsDialog::buildGeneralPage()
{
    auto *container = new QWidget(this);
    auto *outer = new QVBoxLayout(container);
    outer->setContentsMargins(24, 24, 24, 24);
    outer->setSpacing(8);

    auto addRowLabel = [](QGridLayout *layout, int row, const QString &text, QWidget *parent) {
        auto *label = new QLabel(text, parent);
        label->setObjectName(QStringLiteral("fieldLabel"));
        layout->addWidget(label, row, 0);
    };

    outer->addWidget(createSectionTitle(tr("大模型设置")));
    {
        auto *card = createCard();
        auto *grid = new QGridLayout(card);
        grid->setContentsMargins(16, 16, 16, 16);
        grid->setHorizontalSpacing(12);
        grid->setVerticalSpacing(12);

        addRowLabel(grid, 0, tr("大模型选择"), card);
        m_modelCombo = new SettingsComboBox(card);
        m_modelCombo->addItems({ tr("阿里云百炼"), tr("火山引擎") });
        if (m_aiViewModel) m_modelCombo->setCurrentIndex(m_aiViewModel->selectedModel());
        connect(m_modelCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (m_aiViewModel) m_aiViewModel->setSelectedModel(index);
        });
        grid->addWidget(m_modelCombo, 0, 1, 1, 2);

        addRowLabel(grid, 1, QStringLiteral("API Key"), card);
        m_apiKeyEdit = new QLineEdit(card);
        if (m_aiViewModel) m_apiKeyEdit->setText(m_aiViewModel->apiKey());
        grid->addWidget(m_apiKeyEdit, 1, 1);
        auto *saveApi = new QPushButton(tr("保存"), card);
        saveApi->setObjectName(QStringLiteral("primaryButton"));
        connect(saveApi, &QPushButton::clicked, this, [this]() {
            if (m_aiViewModel) m_aiViewModel->setApiKey(m_apiKeyEdit->text());
        });
        grid->addWidget(saveApi, 1, 2);
        outer->addWidget(card);
    }

    outer->addSpacing(8);
    outer->addWidget(createSectionTitle(tr("语言设置")));
    {
        auto *card = createCard();
        auto *grid = new QGridLayout(card);
        grid->setContentsMargins(16, 16, 16, 16);
        grid->setHorizontalSpacing(12);
        addRowLabel(grid, 0, tr("界面语言"), card);
        m_languageCombo = new SettingsComboBox(card);
        m_languageCombo->addItems({ QStringLiteral("中文"), QStringLiteral("English") });
        if (m_languageManager) m_languageCombo->setCurrentIndex(m_languageManager->language() == QStringLiteral("en") ? 1 : 0);
        connect(m_languageCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (m_languageManager) {
                m_languageManager->setLanguage(index == 1 ? QStringLiteral("en") : QStringLiteral("zh_CN"));
            }
        });
        grid->addWidget(m_languageCombo, 0, 1);
        outer->addWidget(card);
    }

    outer->addSpacing(8);
    outer->addWidget(createSectionTitle(tr("截图保存路径")));
    {
        auto *card = createCard();
        auto *grid = new QGridLayout(card);
        grid->setContentsMargins(16, 16, 16, 16);
        grid->setHorizontalSpacing(12);
        addRowLabel(grid, 0, tr("保存路径"), card);
        m_savePathLabel = new QLabel(card);
        m_savePathLabel->setObjectName(QStringLiteral("pathLabel"));
        if (m_storageViewModel) m_savePathLabel->setText(m_storageViewModel->savePath().toLocalFile());
        m_savePathLabel->setMinimumHeight(36);
        m_savePathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        grid->addWidget(m_savePathLabel, 0, 1);
        auto *browse = new QPushButton(tr("浏览"), card);
        browse->setObjectName(QStringLiteral("primaryButton"));
        connect(browse, &QPushButton::clicked, this, [this]() {
            const QString current = m_storageViewModel ? m_storageViewModel->savePath().toLocalFile() : QString();
            const QString dir = QFileDialog::getExistingDirectory(this, tr("选择保存位置"), current);
            if (!dir.isEmpty() && m_storageViewModel) {
                m_storageViewModel->setSavePath(QUrl::fromLocalFile(dir));
                m_savePathLabel->setText(dir);
            }
        });
        grid->addWidget(browse, 0, 2);
        outer->addWidget(card);
    }

    outer->addSpacing(8);
    outer->addWidget(createSectionTitle(tr("GIF 录制")));
    {
        auto *card = createCard();
        auto *grid = new QGridLayout(card);
        grid->setContentsMargins(16, 16, 16, 16);
        grid->setHorizontalSpacing(12);
        addRowLabel(grid, 0, tr("体积预设"), card);
        m_gifPresetCombo = new SettingsComboBox(card);
        m_gifPresetCombo->addItems({ tr("高质量"), tr("均衡"), tr("小体积") });
        if (m_gifRecordViewModel) m_gifPresetCombo->setCurrentIndex(m_gifRecordViewModel->qualityPreset());
        connect(m_gifPresetCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (m_gifRecordViewModel) m_gifRecordViewModel->setQualityPreset(index);
        });
        grid->addWidget(m_gifPresetCombo, 0, 1);
        outer->addWidget(card);
    }

    outer->addSpacing(8);
    outer->addWidget(createSectionTitle(tr("OCR 自检")));
    {
        auto *card = createCard();
        auto *v = new QVBoxLayout(card);
        v->setContentsMargins(12, 12, 12, 12);
        v->setSpacing(10);

        m_ocrSelfCheckText = new QTextEdit(card);
        m_ocrSelfCheckText->setReadOnly(true);
        m_ocrSelfCheckText->setMinimumHeight(120);
        m_ocrSelfCheckText->setText(m_ocrViewModel && !m_ocrViewModel->selfCheckText().isEmpty()
                                        ? m_ocrViewModel->selfCheckText()
                                        : tr("点击“一键自检”查看 OCR 环境状态。"));
        v->addWidget(m_ocrSelfCheckText);

        auto *row = new QHBoxLayout;
        auto *selfCheck = new QPushButton(tr("一键自检"), card);
        selfCheck->setObjectName(QStringLiteral("primaryButton"));
        connect(selfCheck, &QPushButton::clicked, this, [this]() {
            if (m_ocrViewModel) m_ocrSelfCheckText->setText(m_ocrViewModel->runSelfCheck());
        });
        row->addWidget(selfCheck);

        auto *openTess = new QPushButton(tr("打开 tessdata"), card);
        openTess->setObjectName(QStringLiteral("secondaryButton"));
        connect(openTess, &QPushButton::clicked, this, [this]() {
            if (m_ocrViewModel && !m_ocrViewModel->openTessdataFolder()) {
                m_ocrSelfCheckText->setText(m_ocrViewModel->runSelfCheck());
            }
        });
        row->addWidget(openTess);
        row->addStretch();
        v->addLayout(row);
        outer->addWidget(card);
    }

    outer->addStretch();
    return container;
}

QWidget *SettingsDialog::createHotkeyRow(const QString &labelText, int actionId, const QString &seq)
{
    auto *rowWrap = new QWidget(this);
    auto *row = new QHBoxLayout(rowWrap);
    row->setContentsMargins(0, 12, 0, 12);
    row->setSpacing(8);

    auto *label = new QLabel(labelText, rowWrap);
    label->setObjectName(QStringLiteral("fieldLabel"));
    label->setFixedWidth(120);
    row->addWidget(label);

    auto *keysWrap = new QWidget(rowWrap);
    auto *keysLayout = new QHBoxLayout(keysWrap);
    keysLayout->setContentsMargins(0, 0, 0, 0);
    keysLayout->setSpacing(4);
    const QStringList parts = seq.split('+', Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        auto *badge = new QLabel(part, keysWrap);
        badge->setObjectName(QStringLiteral("keyBadge"));
        keysLayout->addWidget(badge);
    }
    keysLayout->addStretch();
    row->addWidget(keysWrap, 1);

    auto *edit = new QPushButton(tr("编辑"), rowWrap);
    edit->setObjectName(QStringLiteral("hotkeyEdit"));
    connect(edit, &QPushButton::clicked, this, [this, actionId, labelText]() {
        if (!m_globalShortcut) {
            return;
        }
        HotkeyRecordDialog dialog(labelText, actionId, shortcutForAction(actionId), m_globalShortcut, this);
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }
        const QString seq = dialog.recordedSequence();
        if (seq.isEmpty()) {
            return;
        }
        if (!m_globalShortcut->updateShortcut(actionId, seq)) {
            QMessageBox::warning(this, tr("快捷键更新失败"), tr("快捷键「%1」注册失败，请换一个组合。").arg(seq));
            return;
        }
        refreshHotkeyPage();
    });
    row->addWidget(edit);
    return rowWrap;
}

QWidget *SettingsDialog::buildHotkeyPage()
{
    m_hotkeyPage = new QWidget(this);
    auto *outer = new QVBoxLayout(m_hotkeyPage);
    outer->setContentsMargins(24, 24, 24, 24);
    outer->setSpacing(8);

    outer->addWidget(createSectionTitle(tr("全局快捷键")));
    auto *card = createCard();
    card->setObjectName(QStringLiteral("hotkeySectionCard"));
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(16, 0, 16, 0);
    cardLayout->setSpacing(0);
    outer->addWidget(card);

    outer->addSpacing(8);
    outer->addWidget(createSectionTitle(tr("说明")));
    auto *noteCard = createCard();
    auto *noteLayout = new QVBoxLayout(noteCard);
    noteLayout->setContentsMargins(16, 12, 16, 12);
    auto *note = new QLabel(tr("修改快捷键后会立即保存并重新注册。建议使用包含 Ctrl/Alt/Shift 的组合，以避免与系统或其他软件冲突。"), noteCard);
    note->setObjectName(QStringLiteral("noteText"));
    note->setWordWrap(true);
    noteLayout->addWidget(note);
    outer->addWidget(noteCard);

    outer->addStretch();
    refreshHotkeyPage();
    return m_hotkeyPage;
}

QString SettingsDialog::shortcutForAction(int actionId) const
{
    if (!m_globalShortcut) {
        return {};
    }
    switch (actionId) {
    case 1: return m_globalShortcut->shortcutScreenshot();
    case 2: return m_globalShortcut->shortcutScreenshotSave();
    case 3: return m_globalShortcut->shortcutSticky();
    case 4: return m_globalShortcut->shortcutToggle();
    default: return {};
    }
}

void SettingsDialog::refreshHotkeyPage()
{
    if (!m_hotkeyPage) {
        return;
    }

    auto *card = m_hotkeyPage->findChild<QFrame *>(QStringLiteral("hotkeySectionCard"));
    if (!card) {
        return;
    }

    auto *layout = qobject_cast<QVBoxLayout *>(card->layout());
    if (!layout) {
        return;
    }

    while (layout->count() > 0) {
        QLayoutItem *item = layout->takeAt(0);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    const struct RowInfo { const char *label; int actionId; } rows[] = {
        { QT_TR_NOOP("截图"), 1 },
        { QT_TR_NOOP("截图并保存"), 2 },
        { QT_TR_NOOP("贴图到桌面"), 3 },
        { QT_TR_NOOP("显示/隐藏窗口"), 4 }
    };

    for (int i = 0; i < 4; ++i) {
        layout->addWidget(createHotkeyRow(tr(rows[i].label), rows[i].actionId, shortcutForAction(rows[i].actionId)));
        if (i != 3) {
            auto *divider = new QFrame(card);
            divider->setObjectName(QStringLiteral("settingsRowDivider"));
            divider->setFixedHeight(1);
            layout->addWidget(divider);
        }
    }
}
