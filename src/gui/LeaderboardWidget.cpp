// OOP: GUI Presentation Layer (leaderboard screen)
#include "gui/LeaderboardWidget.h"
#include "gui/SessionContext.h"
#include "core/User.h"
#include "progress/ProgressService.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QTableWidget>
#include <QLabel>
#include <QHeaderView>
#include <QString>

namespace gui {

LeaderboardWidget::LeaderboardWidget(SessionContext& context, QWidget* parent)
    : QWidget(parent)
    , context_(context) {
    buildUi();
    onFilterChanged();
}

void LeaderboardWidget::buildUi() {
    QVBoxLayout* root = new QVBoxLayout(this);
    
    QHBoxLayout* filters = new QHBoxLayout;
    filters->addWidget(new QLabel(tr("Scope:"), this));
    scopeCombo_ = new QComboBox(this);
    scopeCombo_->addItem(tr("Global"), QStringLiteral("global"));
    scopeCombo_->addItem(tr("My college"), QStringLiteral("college"));
    scopeCombo_->addItem(tr("My branch"), QStringLiteral("branch"));
    filters->addWidget(scopeCombo_);
    
    filters->addWidget(new QLabel(tr("Window:"), this));
    windowCombo_ = new QComboBox(this);
    windowCombo_->addItem(tr("All time"), QStringLiteral("alltime"));
    windowCombo_->addItem(tr("Week"), QStringLiteral("week"));
    windowCombo_->addItem(tr("Month"), QStringLiteral("month"));
    filters->addWidget(windowCombo_);
    
    filters->addWidget(new QLabel(tr("Ranking:"), this));
    rankingCombo_ = new QComboBox(this);
    rankingCombo_->addItem(tr("Score"), QStringLiteral("score"));
    rankingCombo_->addItem(tr("Solved"), QStringLiteral("solved"));
    rankingCombo_->addItem(tr("Streak"), QStringLiteral("streak"));
    filters->addWidget(rankingCombo_);
    filters->addStretch(1);
    root->addLayout(filters);
    
    table_ = new QTableWidget(this);
    table_->setColumnCount(5);
    table_->setHorizontalHeaderLabels({tr("Rank"), tr("Name"), tr("Score"),
                                       tr("Solved"), tr("Streak")});
    table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setAlternatingRowColors(true);
    root->addWidget(table_, 1);
    
    rankLabel_ = new QLabel(this);
    rankLabel_->setStyleSheet(QStringLiteral("font-weight: bold;"));
    root->addWidget(rankLabel_);
    
    connect(scopeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LeaderboardWidget::onFilterChanged);
    connect(windowCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LeaderboardWidget::onFilterChanged);
    connect(rankingCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LeaderboardWidget::onFilterChanged);
}

void LeaderboardWidget::setUser(const core::UserPtr& user) {
    user_ = user;
    refresh();
}

void LeaderboardWidget::onFilterChanged() {
    refresh();
}

void LeaderboardWidget::refresh() {
    progress::LeaderboardQuery query;
    query.scope = scopeCombo_->currentData().toString().toStdString();
    query.window = windowCombo_->currentData().toString().toStdString();
    query.ranking = rankingCombo_->currentData().toString().toStdString();
    query.topN = 25;
    
    std::vector<core::ScoreboardEntry> entries = context_.progress().leaderboard(query);
    table_->setRowCount(static_cast<int>(entries.size()));
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const core::ScoreboardEntry& entry = entries[i];
        table_->setItem(static_cast<int>(i), 0,
                        new QTableWidgetItem(QString::number(entry.rank)));
        table_->setItem(static_cast<int>(i), 1,
                        new QTableWidgetItem(QString::fromStdString(entry.name)));
        table_->setItem(static_cast<int>(i), 2,
                        new QTableWidgetItem(QString::number(entry.score, 'f', 2)));
        table_->setItem(static_cast<int>(i), 3,
                        new QTableWidgetItem(QString::number(entry.solved)));
        table_->setItem(static_cast<int>(i), 4,
                        new QTableWidgetItem(QString::number(entry.streak)));
    }
    
    if (user_) {
        const int rank = context_.progress().userRank(user_->id(), query);
        rankLabel_->setText(rank > 0 ? tr("Your rank: #%1").arg(rank)
                                     : tr("You are not ranked on this board yet."));
    } else {
        rankLabel_->clear();
    }
}

} // namespace gui
