import QtQuick
import net.asivery.AppLoad 1.0

Rectangle {
    id: root
    anchors.fill: parent
    color: "white"

    // Must match backend/main.cpp
    readonly property int msgNewGame: 1
    readonly property int msgSave: 2
    readonly property int msgReady: 100
    readonly property int msgGame: 101
    readonly property int msgRecognize: 3
    readonly property int msgRecognized: 102
    readonly property int msgEraser: 103

    property string level: "Easy"   // level of the next new game
    property var givens: []         // 81 digits, 0 for empty
    property var solution: []
    property var entries: []        // digits entered by the player, 0 for empty
    property int selected: -1
    property bool checking: false
    property bool generating: true
    property bool erasing: false    // the eraser end of the pen is near the screen

    readonly property bool hasGame: givens.length === 81
    readonly property bool solved: hasGame && givens.every((given, cell) => (given || entries[cell]) === solution[cell])
    readonly property int selectedDigit: selected < 0 ? 0 : digitAt(selected)

    // Called by AppLoad when the app is closed; stops the backend
    function unloading() {
        backend.terminate()
    }

    function digitAt(cell) {
        return givens[cell] || entries[cell] || 0
    }

    function sameUnit(a, b) {
        const box = cell => Math.floor(cell / 27) * 3 + Math.floor((cell % 9) / 3)
        return Math.floor(a / 9) === Math.floor(b / 9) || a % 9 === b % 9 || box(a) === box(b)
    }

    // Whether another cell in the same row, column or box has the same digit
    function conflicts(cell) {
        const digit = digitAt(cell)
        if (!digit) {
            return false
        }
        for (let other = 0; other < 81; other++) {
            if (other !== cell && sameUnit(cell, other) && digitAt(other) === digit) {
                return true
            }
        }
        return false
    }

    function placed(digit) {
        let count = 0
        for (let cell = 0; cell < 81; cell++) {
            count += digitAt(cell) === digit
        }
        return count
    }

    function setEntry(cell, digit) {
        if (cell < 0 || givens[cell] || solved || entries[cell] === digit) {
            return
        }
        const updated = entries.slice()
        updated[cell] = digit
        entries = updated
        backend.sendMessage(msgSave, entries.join(""))
    }

    function enter(digit) {
        setEntry(selected, digit)
    }

    function newGame() {
        generating = true
        backend.sendMessage(msgNewGame, level.toLowerCase())
    }

    function loadGame(json) {
        const game = JSON.parse(json)
        const digits = text => Array.from(text, Number)
        givens = digits(game.givens)
        solution = digits(game.solution)
        entries = digits(game.entries)
        level = game.level.charAt(0).toUpperCase() + game.level.slice(1)
        selected = -1
        checking = false
        generating = false
    }

    AppLoad {
        id: backend
        applicationID: "sudoku"
        onMessageReceived: (type, contents) => {
            switch (type) {
            case root.msgReady:
                root.newGame()
                break
            case root.msgGame:
                ink.reset()
                root.loadGame(contents)
                break
            case root.msgRecognized: {
                const space = contents.indexOf(" ")
                const cell = parseInt(contents.substring(0, space))
                const digit = contents.substring(space + 1)
                ink.clearCell(cell)
                if (digit === "!") {
                    root.setEntry(cell, 0)
                } else if (digit !== "") {
                    root.setEntry(cell, parseInt(digit))
                }
                break
            }
            case root.msgEraser:
                root.erasing = contents === "1"
                break
            }
        }
    }

    // Level of the next game, and actions
    Row {
        id: toolbar
        anchors {
            top: parent.top
            left: parent.left
            margins: 16
        }
        height: 72
        spacing: 12

        Repeater {
            model: ["Easy", "Normal", "Hard"]
            delegate: Button {
                text: modelData
                checked: root.level === modelData
                onClicked: root.level = modelData
            }
        }
        Item { width: 12; height: 1 }
        Button {
            id: newButton
            // Ask again before throwing away a game in progress
            property bool confirming: false
            text: confirming ? "Sure?" : "New"
            checked: confirming
            enabled: !root.generating
            onClicked: {
                if (root.hasGame && !root.solved && !confirming && root.entries.some(digit => digit)) {
                    confirming = true
                    confirmTimer.restart()
                } else {
                    confirming = false
                    root.newGame()
                }
            }
            Timer {
                id: confirmTimer
                interval: 3000
                onTriggered: newButton.confirming = false
            }
        }
        Button {
            text: "Check"
            enabled: root.hasGame
            checked: root.checking
            onClicked: root.checking = !root.checking
        }
    }

    Item {
        id: board
        anchors {
            top: toolbar.bottom
            topMargin: 24
            horizontalCenter: parent.horizontalCenter
        }
        readonly property real cellSize: Math.floor(Math.min(root.width - 32, root.height * 0.6) / 9)
        width: cellSize * 9
        height: cellSize * 9
        visible: root.hasGame

        Repeater {
            model: 81
            delegate: Rectangle {
                id: cell
                readonly property int digit: root.digitAt(index)
                readonly property bool given: root.givens[index] !== 0
                readonly property bool wrong: root.checking && !given && digit !== 0 && digit !== root.solution[index]

                x: (index % 9) * board.cellSize
                y: Math.floor(index / 9) * board.cellSize
                width: board.cellSize
                height: board.cellSize
                border.width: 1
                border.color: "black"
                color: index === root.selected ? "#a0a0a0"
                    : root.selectedDigit !== 0 && digit === root.selectedDigit ? "#c8c8c8"
                    : root.selected >= 0 && root.sameUnit(index, root.selected) ? "#e8e8e8"
                    : "white"

                Text {
                    anchors.centerIn: parent
                    text: cell.digit || ""
                    font.pixelSize: board.cellSize * 0.62
                    font.bold: cell.given
                }
                // Duplicate digit in a row, column or box
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 6
                    visible: root.conflicts(index)
                    color: "transparent"
                    border.width: 3
                    radius: width / 2
                }
                // Strike out wrong digits when checking
                Rectangle {
                    visible: cell.wrong
                    anchors.centerIn: parent
                    width: parent.width * 1.1
                    height: 4
                    rotation: -45
                    color: "black"
                }
                TapHandler {
                    onTapped: root.selected = index
                }
            }
        }

        // Handwriting: pen strokes write a digit in the cell they start in.
        // xochitl delivers the pen as plain mouse events and touches as
        // synthesized ones, which fall through to the cells' TapHandlers.
        Canvas {
            id: ink
            anchors.fill: parent

            property var strokes: ({})    // cell -> list of strokes, in cell-local coordinates
            property var current: null    // { cell, points } or { erasing, lastErased }
            property var pendingSegments: []
            property bool pendingClear: false

            function reset() {
                strokes = {}
                current = null
                pendingSegments = []
                pendingClear = true
                recognizeTimer.stop()
                requestPaint()
            }

            function cellAt(x, y) {
                const column = Math.floor(x / board.cellSize)
                const row = Math.floor(y / board.cellSize)
                return column < 0 || row < 0 || column > 8 || row > 8 ? -1 : row * 9 + column
            }

            function clearCell(cell) {
                delete strokes[cell]
                const x = (cell % 9) * board.cellSize
                const y = Math.floor(cell / 9) * board.cellSize
                pendingSegments.push({ clear: true, x: x, y: y })
                markDirty(Qt.rect(x, y, board.cellSize, board.cellSize))
            }

            function erase(x, y) {
                const cell = cellAt(x, y)
                if (cell < 0 || cell === current.lastErased) {
                    return
                }
                current.lastErased = cell
                if (recognizeTimer.cell === cell) {
                    recognizeTimer.stop()
                }
                clearCell(cell)
                root.setEntry(cell, 0)
            }

            function begin(x, y) {
                if (root.erasing) {
                    current = { erasing: true, lastErased: -1 }
                    erase(x, y)
                    return
                }
                const cell = cellAt(x, y)
                if (cell < 0) {
                    current = null
                    return
                }
                // Moving to another cell: recognize the previous one right away
                if (recognizeTimer.running && recognizeTimer.cell !== cell) {
                    recognizeTimer.stop()
                    recognize(recognizeTimer.cell)
                }
                recognizeTimer.stop()
                root.selected = cell
                current = root.givens[cell] ? null : { cell: cell, points: [Qt.point(x, y)] }
            }

            function extend(x, y) {
                if (current === null) {
                    return
                }
                if (current.erasing) {
                    erase(x, y)
                    return
                }
                // Keep the ink inside the cell it started in
                const left = (current.cell % 9) * board.cellSize
                const top = Math.floor(current.cell / 9) * board.cellSize
                x = Math.max(left, Math.min(left + board.cellSize - 1, x))
                y = Math.max(top, Math.min(top + board.cellSize - 1, y))

                const last = current.points[current.points.length - 1]
                current.points.push(Qt.point(x, y))
                pendingSegments.push({ from: last, to: Qt.point(x, y) })
                markDirty(Qt.rect(Math.min(last.x, x) - 4, Math.min(last.y, y) - 4,
                                  Math.abs(last.x - x) + 8, Math.abs(last.y - y) + 8))
            }

            function end() {
                if (current === null || current.erasing) {
                    current = null
                    return
                }
                const cell = current.cell
                const left = (cell % 9) * board.cellSize
                const top = Math.floor(cell / 9) * board.cellSize
                if (!strokes[cell]) {
                    strokes[cell] = []
                }
                strokes[cell].push(current.points.map(p => Qt.point(p.x - left, p.y - top)))
                current = null
                recognizeTimer.cell = cell
                recognizeTimer.restart()
            }

            function recognize(cell) {
                const cellStrokes = strokes[cell]
                if (!cellStrokes) {
                    return
                }
                const encoded = cellStrokes.map(stroke => stroke.map(p => Math.round(p.x) + "," + Math.round(p.y)).join(" "))
                backend.sendMessage(root.msgRecognize, cell + " " + Math.round(board.cellSize) + ";" + encoded.join(";"))
            }

            onPaint: {
                const ctx = getContext("2d")
                if (pendingClear) {
                    ctx.clearRect(0, 0, width, height)
                    pendingClear = false
                }
                ctx.strokeStyle = "black"
                ctx.lineWidth = 5
                ctx.lineCap = "round"
                for (const segment of pendingSegments) {
                    if (segment.clear) {
                        ctx.clearRect(segment.x, segment.y, board.cellSize, board.cellSize)
                        continue
                    }
                    ctx.beginPath()
                    ctx.moveTo(segment.from.x, segment.from.y)
                    ctx.lineTo(segment.to.x, segment.to.y)
                    ctx.stroke()
                }
                pendingSegments = []
            }

            MouseArea {
                anchors.fill: parent
                preventStealing: true
                onPressed: mouse => {
                    if (mouse.source !== Qt.MouseEventNotSynthesized) {
                        mouse.accepted = false
                        return
                    }
                    ink.begin(mouse.x, mouse.y)
                }
                onPositionChanged: mouse => ink.extend(mouse.x, mouse.y)
                onReleased: ink.end()
                onCanceled: ink.end()
            }

            // Digits can take several strokes; wait for the pen to pause before recognizing
            Timer {
                id: recognizeTimer
                property int cell: -1
                interval: 600
                onTriggered: ink.recognize(cell)
            }
        }

        // Box borders
        Repeater {
            model: 4
            delegate: Item {
                required property int index
                Rectangle {
                    x: index * board.cellSize * 3 - 2
                    y: -2
                    width: 5
                    height: board.height + 4
                    color: "black"
                }
                Rectangle {
                    x: -2
                    y: index * board.cellSize * 3 - 2
                    width: board.width + 4
                    height: 5
                    color: "black"
                }
            }
        }
    }

    Text {
        id: status
        anchors {
            top: board.bottom
            topMargin: 20
            horizontalCenter: parent.horizontalCenter
        }
        // Fixed height so the keypad doesn't move when the text changes
        height: 56
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: 36
        font.bold: root.solved
        text: root.generating ? "Generating…"
            : root.solved ? "Solved!"
            : root.selected >= 0 && root.givens[root.selected] ? "That number is given"
            : root.selected < 0 ? "Write in a square, or tap it then a number"
            : ""
    }

    // Keypad
    Grid {
        id: keypad
        anchors {
            top: status.bottom
            topMargin: 20
            horizontalCenter: parent.horizontalCenter
        }
        columns: 5
        spacing: 12
        visible: root.hasGame

        readonly property real keyWidth: (board.width - 4 * spacing) / 5

        Repeater {
            model: 9
            delegate: Button {
                readonly property int digit: index + 1
                width: keypad.keyWidth
                height: 128
                text: digit
                fontSize: 56
                // All nine are placed
                enabled: root.placed(digit) < 9
                onClicked: root.enter(digit)
            }
        }
        Button {
            width: keypad.keyWidth
            height: 128
            text: "Erase"
            onClicked: root.enter(0)
        }
    }

    component Button: Rectangle {
        id: button
        property string text
        property bool checked: false
        property bool enabled: true
        property int fontSize: 28
        signal clicked()

        width: label.implicitWidth + 40
        height: parent.height
        radius: 8
        border.width: 3
        border.color: enabled ? "black" : "#b0b0b0"
        color: checked ? "black" : "white"

        Text {
            id: label
            anchors.centerIn: parent
            text: button.text
            font.pixelSize: button.fontSize
            color: button.checked ? "white" : button.enabled ? "black" : "#b0b0b0"
        }
        TapHandler {
            enabled: button.enabled
            onTapped: button.clicked()
        }
    }
}
