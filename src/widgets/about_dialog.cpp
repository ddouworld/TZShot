#include "widgets/about_dialog.h"

#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScreen>
#include <QVBoxLayout>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("关于 TZshot"));
    setModal(false);
    setFixedSize(400, 350);
    setStyleSheet(QStringLiteral("background:#F8FAFC;"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(32, 32, 32, 24);
    layout->setSpacing(0);

    auto *logoWrap = new QWidget(this);
    auto *logoLayout = new QHBoxLayout(logoWrap);
    logoLayout->setContentsMargins(0, 0, 0, 0);
    auto *logo = new QLabel(QStringLiteral("T"), logoWrap);
    logo->setAlignment(Qt::AlignCenter);
    logo->setFixedSize(72, 72);
    logo->setStyleSheet(QStringLiteral(
        "background:#3B82F6; color:#FFFFFF; border-radius:18px; font-size:40px; font-weight:700;"));
    logoLayout->addStretch(1);
    logoLayout->addWidget(logo);
    logoLayout->addStretch(1);
    layout->addWidget(logoWrap);

    layout->addSpacing(16);

    auto *title = new QLabel(QStringLiteral("TZshot"), this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(QStringLiteral("font-size:22px; font-weight:700; color:#1E293B;"));
    layout->addWidget(title);

    layout->addSpacing(6);

    auto *version = new QLabel(tr("版本 v1.0.0"), this);
    version->setAlignment(Qt::AlignCenter);
    version->setStyleSheet(QStringLiteral("font-size:13px; color:#64748B;"));
    layout->addWidget(version);

    layout->addSpacing(16);

    auto *divider = new QFrame(this);
    divider->setFrameShape(QFrame::HLine);
    divider->setStyleSheet(QStringLiteral("color:#E2E8F0;"));
    layout->addWidget(divider);

    layout->addSpacing(16);

    auto *desc = new QLabel(tr("简洁高效的截图贴图工具，支持标注、AI 编辑、马赛克等功能。"), this);
    desc->setWordWrap(true);
    desc->setAlignment(Qt::AlignCenter);
    desc->setStyleSheet(QStringLiteral("font-size:13px; color:#475569;"));
    layout->addWidget(desc);

    layout->addStretch(1);

    auto *copyright = new QLabel(tr("© 2025 TZshot. All rights reserved."), this);
    copyright->setAlignment(Qt::AlignCenter);
    copyright->setStyleSheet(QStringLiteral("font-size:11px; color:#94A3B8;"));
    layout->addWidget(copyright);

    layout->addSpacing(8);

    auto *buttonWrap = new QWidget(this);
    auto *buttonLayout = new QHBoxLayout(buttonWrap);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    auto *closeButton = new QPushButton(tr("关闭"), buttonWrap);
    closeButton->setFixedSize(88, 34);
    closeButton->setStyleSheet(QStringLiteral(
        "QPushButton { background:#3B82F6; color:white; border:none; border-radius:8px; font-size:13px; }"
        "QPushButton:hover { background:#2563EB; }"));
    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(closeButton);
    buttonLayout->addStretch(1);
    layout->addWidget(buttonWrap);
}

void AboutDialog::showAndActivate()
{
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        const QRect work = screen->availableGeometry();
        move(work.center() - rect().center());
    }
    show();
    raise();
    activateWindow();
}
