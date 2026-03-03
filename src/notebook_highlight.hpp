#pragma once
// MarkdownHighlighter — QSyntaxHighlighter replacement for GtkSourceView markdown (T011)
// Proximity rendering: cursorPositionChanged → rehighlightBlock on ±2 blocks of cursor

#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QColor>
#include <vector>

class MarkdownHighlighter : public QSyntaxHighlighter
{
public:
    explicit MarkdownHighlighter(QTextDocument *parent = nullptr);

    // Proximity rendering: rehighlight ±2 blocks around block at blockNumber
    void proximityRehighlight(int blockNumber);

    // Toggle proximity rendering (hide markup away from cursor, show near it)
    void setProximityEnabled(bool enabled);
    bool isProximityEnabled() const { return m_proximityEnabled; }

    // Tell the highlighter where the cursor is (for proximity rendering)
    void setCursorBlock(int blockNumber);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct Rule {
        QRegularExpression pattern;
        QTextCharFormat    format;
        bool               hideAwayFromCursor = false; // markup to hide when far from cursor
    };

    std::vector<Rule> m_rules;
    bool m_proximityEnabled = true;
    int  m_cursorBlock      = -1;

    void buildRules();

    // Returns true if the current block is within ±2 of the cursor
    bool isNearCursor() const;
};

inline MarkdownHighlighter::MarkdownHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    buildRules();
}

inline void MarkdownHighlighter::buildRules()
{
    m_rules.clear();

    // Headings (ATX: # through ######)
    {
        Rule r;
        r.pattern = QRegularExpression(QStringLiteral("^#{1,6}\\s.*$"));
        r.format.setFontWeight(QFont::Bold);
        r.format.setForeground(QColor(QStringLiteral("#2196F3")));
        r.hideAwayFromCursor = false;
        m_rules.push_back(r);
    }

    // Bold **text** or __text__
    {
        Rule r;
        r.pattern = QRegularExpression(QStringLiteral("(\\*\\*|__)(?=\\S)(.+?)(?<=\\S)\\1"));
        r.format.setFontWeight(QFont::Bold);
        r.hideAwayFromCursor = false;
        m_rules.push_back(r);
    }

    // Italic *text* or _text_ (not bold)
    {
        Rule r;
        r.pattern = QRegularExpression(QStringLiteral("(?<!\\*)(\\*|_)(?!\\*)(?=\\S)(.+?)(?<=\\S)\\1(?!\\*)"));
        r.format.setFontItalic(true);
        r.hideAwayFromCursor = false;
        m_rules.push_back(r);
    }

    // Inline code `code`
    {
        Rule r;
        r.pattern = QRegularExpression(QStringLiteral("`[^`]+`"));
        r.format.setFontFamily(QStringLiteral("monospace"));
        r.format.setForeground(QColor(QStringLiteral("#E91E63")));
        r.hideAwayFromCursor = false;
        m_rules.push_back(r);
    }

    // Blockquote >
    {
        Rule r;
        r.pattern = QRegularExpression(QStringLiteral("^\\s*>.*$"));
        r.format.setForeground(QColor(QStringLiteral("#9E9E9E")));
        r.hideAwayFromCursor = false;
        m_rules.push_back(r);
    }

    // Markdown link [text](url)
    {
        Rule r;
        r.pattern = QRegularExpression(QStringLiteral("\\[([^\\]]+)\\]\\(([^\\)]+)\\)"));
        r.format.setForeground(QColor(QStringLiteral("#4CAF50")));
        r.format.setFontUnderline(true);
        r.hideAwayFromCursor = false;
        m_rules.push_back(r);
    }

    // Fenced code blocks ```
    {
        Rule r;
        r.pattern = QRegularExpression(QStringLiteral("^```.*$"));
        r.format.setFontFamily(QStringLiteral("monospace"));
        r.format.setForeground(QColor(QStringLiteral("#FF9800")));
        r.hideAwayFromCursor = false;
        m_rules.push_back(r);
    }

    // Horizontal rules (--- / ***)
    {
        Rule r;
        r.pattern = QRegularExpression(QStringLiteral("^(---+|\\*\\*\\*+|___+)$"));
        r.format.setForeground(QColor(QStringLiteral("#9E9E9E")));
        r.hideAwayFromCursor = false;
        m_rules.push_back(r);
    }

    // Markup characters for proximity rendering (make transparent when far from cursor)
    // E.g. the ** in **bold** can be hidden away from cursor
    {
        Rule r;
        r.pattern = QRegularExpression(QStringLiteral("(\\*{1,2}|_{1,2}|`|^#{1,6}\\s)"));
        QTextCharFormat transparentFmt;
        transparentFmt.setForeground(Qt::transparent);
        r.format = transparentFmt;
        r.hideAwayFromCursor = true; // only applied when proximity rendering is on and far from cursor
        m_rules.push_back(r);
    }
}

inline void MarkdownHighlighter::highlightBlock(const QString &text)
{
    int blockNum = currentBlock().blockNumber();
    bool near = isNearCursor();

    for (const auto &rule : m_rules) {
        // Skip "hideAwayFromCursor" rules when near cursor or proximity is off
        if (rule.hideAwayFromCursor) {
            if (!m_proximityEnabled || near)
                continue; // show markup (don't apply transparent format)
        }

        auto it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            auto match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}

inline bool MarkdownHighlighter::isNearCursor() const
{
    if (m_cursorBlock < 0) return true;
    int blockNum = currentBlock().blockNumber();
    return std::abs(blockNum - m_cursorBlock) <= 2;
}

inline void MarkdownHighlighter::proximityRehighlight(int blockNumber)
{
    setCursorBlock(blockNumber);
    // Rehighlight ±2 blocks
    QTextDocument *doc = document();
    if (!doc) return;
    int first = std::max(0, blockNumber - 2);
    int last  = std::min(doc->blockCount() - 1, blockNumber + 2);
    for (int i = first; i <= last; ++i) {
        QTextBlock b = doc->findBlockByNumber(i);
        if (b.isValid())
            rehighlightBlock(b);
    }
}

inline void MarkdownHighlighter::setProximityEnabled(bool enabled)
{
    m_proximityEnabled = enabled;
    rehighlight();
}

inline void MarkdownHighlighter::setCursorBlock(int blockNumber)
{
    m_cursorBlock = blockNumber;
}
