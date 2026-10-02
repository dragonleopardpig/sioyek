// Fails on a tooltip wide enough to read as a banner rather than as help.
//
// The note editor's tooltip was once a single 976-pixel line. Qt pops a tooltip
// about a second after the pointer settles and takes it away when the pointer
// moves, so that arrived as a banner flashing across the screen and vanishing.
// It was reported several times as "a popup that appears and closes itself",
// and was hard to find precisely because nothing in the code shows it -- Qt
// does, on a hover timer.
//
// Qt lays a plain-text tooltip out without wrapping until it exceeds the
// screen, so its natural width is simply the widest line. Measure that for
// every setToolTip() string in pdf_viewer/.
//
// Build and run from the repository root:
//   g++ -std=c++17 -fPIC scripts/tools/check_tooltip_shape.cpp \
//       -o /tmp/check_tooltip_shape \
//       $(pkg-config --cflags --libs Qt6Widgets Qt6Gui Qt6Core)
//   QT_QPA_PLATFORM=offscreen /tmp/check_tooltip_shape .
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontMetrics>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>
#include <QToolTip>
#include <cstdio>

static const int kMaxWidthPx = 400;

// Concatenates the adjacent string literals of one setToolTip(...) argument and
// turns the escapes we actually use back into characters.
static QString literalsOf(const QString& arg) {
    static const QRegularExpression re(R"RX("((?:[^"\\]|\\.)*)")RX");
    QString out;
    auto it = re.globalMatch(arg);
    while (it.hasNext()) out += it.next().captured(1);
    out.replace("\\n", "\n");
    out.replace("\\t", "\t");
    out.replace("\\\"", "\"");
    out.replace("\\\\", "\\");
    return out;
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    const QString root = argc > 1 ? QString::fromUtf8(argv[1]) : QStringLiteral(".");

    QDir dir(root + "/pdf_viewer");
    const QStringList sources = dir.entryList({ "*.cpp" }, QDir::Files, QDir::Name);
    if (sources.isEmpty()) {
        printf("no sources under %s/pdf_viewer\n", root.toUtf8().constData());
        return 2;
    }

    const QFontMetrics fm(QToolTip::font());
    int checked = 0, failures = 0;

    for (const QString& name : sources) {
        QFile f(dir.filePath(name));
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        const QString text = QTextStream(&f).readAll();

        int from = 0;
        while ((from = text.indexOf("setToolTip", from)) >= 0) {
            int open = text.indexOf('(', from);
            if (open < 0) break;
            int depth = 0, i = open;
            for (; i < text.size(); i++) {
                if (text[i] == '(') depth++;
                else if (text[i] == ')' && --depth == 0) break;
            }
            const QString body = literalsOf(text.mid(open + 1, i - open - 1));
            if (!body.isEmpty()) {
                const QStringList lines = body.split('\n');
                int width = 0;
                for (const QString& l : lines) width = qMax(width, (int)fm.horizontalAdvance(l));
                const int line = text.left(from).count('\n') + 1;
                const bool wide = width > kMaxWidthPx;
                checked++;
                if (wide) failures++;
                printf("  %s %4dpx x %d line(s)  pdf_viewer/%s:%d\n",
                       wide ? "WIDE" : "ok  ", width, (int)lines.size(),
                       name.toUtf8().constData(), line);
            }
            from = i;
        }
    }

    if (checked == 0) {
        printf("no setToolTip() calls found\n");
        return 0;
    }
    if (failures) {
        printf("\n%d of %d tooltip(s) wider than %dpx. Qt shows a tooltip on a hover\n"
               "timer and hides it again, so a wide single line reads as a banner\n"
               "flashing on screen rather than as help. Break it across lines.\n",
               failures, checked, kMaxWidthPx);
        return 1;
    }
    printf("\nPASS: %d tooltip(s), all at or under %dpx wide.\n", checked, kMaxWidthPx);
    return 0;
}
