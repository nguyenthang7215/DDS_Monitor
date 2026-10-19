// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Theme 1.0

ColumnLayout {
    id: history
    required property var ui
    readonly property var rows: task3Core.events.filter(e => (stateFilter.currentIndex === 0 || e.state === stateFilter.currentText) && (!search.text || (e.source + " " + e.message).toLowerCase().includes(search.text.toLowerCase())))
    spacing: 14
    RowLayout {
        Layout.fillWidth: true
        ColumnLayout {
            Layout.fillWidth: true
            Label {
                text: "Cảnh báo & lịch sử";
                font.pixelSize: 24;
                font.bold: true;
                color: Theme.eProsimaDarkBlue
            }
            Label {
                text: "Theo dõi cảnh báo đang tồn tại và các sự kiện đã khôi phục";
                color: Theme.midGrey;
                wrapMode: Text.Wrap;
                Layout.fillWidth: true
            }
        }
        Button {
            text: "Cảnh báo DDS gốc";
            onClicked: ui.openDdsWorkspace("")
        }
    }
    RowLayout {
        Layout.fillWidth: true
        TextField {
            id: search;
            Layout.fillWidth: true;
            placeholderText: "Tìm đối tượng / điều kiện trong dữ liệu đã tải"
        }
        ComboBox {
            id: stateFilter;
            model: ["Tất cả", "Active", "Recovered"];
            Layout.preferredWidth: 160
        }
    }
    Label {
        text: "Đã tải " + task3Core.events.length + " bản ghi · đang hiển thị " + history.rows.length + ". Bộ lọc áp dụng trên các bản ghi đã tải.";
        color: Theme.midGrey;
        font.pixelSize: 12;
        wrapMode: Text.Wrap;
        Layout.fillWidth: true
    }
    Frame {
        Layout.fillWidth: true;
        Layout.fillHeight: true
        padding: 1
        background: Rectangle {
            color: "white";
            radius: 4;
            border.color: Theme.lightGrey
        }
        ListView {
            id: events
            anchors.fill: parent
            clip: true
            model: history.rows
            spacing: 1
            ScrollBar.vertical: ScrollBar {}
            header: Rectangle {
                width: events.width;
                height: 40;
                color: Theme.eProsimaDarkBlue
                RowLayout {
                    anchors.fill: parent;
                    anchors.margins: 12
                    Label {
                        text: "TRẠNG THÁI / ĐỐI TƯỢNG / ĐIỀU KIỆN";
                        color: "white";
                        Layout.fillWidth: true
                    }
                    Label {
                        text: "THỜI ĐIỂM · GIỜ MÁY MASTER";
                        color: "white";
                        Layout.minimumWidth: 280;
                        Layout.maximumWidth: 280;
                        font.pixelSize: 13
                    }
                }
            }
            delegate: ItemDelegate {
                id: eventRow
                required property var modelData
                required property int index
                readonly property color severityColor: modelData.severity === "error" || modelData.severity === "critical" ? "#b33030" : modelData.severity === "warning" ? "#9a6700" : Theme.eProsimaDarkBlue
                width: events.width
                height: eventContent.implicitHeight + 28
                leftPadding: 12;
                rightPadding: 12
                background: Rectangle {
                    color: eventRow.hovered ? "#f0f4f8" : eventRow.index % 2 ? "#f7f9fb" : "white"
                    border.color: eventRow.activeFocus ? Theme.eProsimaDarkBlue : "transparent"
                    Rectangle {
                        width: 3;
                        height: parent.height;
                        color: eventRow.modelData.state === "Active" ? eventRow.severityColor : "#237a45"
                    }
                }
                contentItem: RowLayout {
                    id: eventContent
                    spacing: 20
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            Label {
                                text: eventRow.modelData.state;
                                font.bold: true;
                                color: eventRow.modelData.state === "Active" ? eventRow.severityColor : "#237a45";
                                padding: 5;
                                background: Rectangle {
                                    radius: 3;
                                    color: eventRow.modelData.state === "Active" ? "#fff4e8" : "#eaf5ef"
                                }
                            }
                            Label {
                                text: eventRow.modelData.severity;
                                color: eventRow.severityColor;
                                font.bold: true
                            }
                            Label {
                                text: eventRow.modelData.kind;
                                color: Theme.midGrey;
                                elide: Text.ElideRight;
                                Layout.fillWidth: true;
                                font.pixelSize: 13
                            }
                        }
                        Label {
                            text: eventRow.modelData.source || "Unknown object";
                            color: Theme.eProsimaDarkBlue;
                            font.bold: true;
                            wrapMode: Text.Wrap;
                            Layout.fillWidth: true
                        }
                        Label {
                            text: eventRow.modelData.message || "Unknown condition";
                            wrapMode: Text.Wrap;
                            Layout.fillWidth: true
                        }
                        Label {
                            visible: !!eventRow.modelData.verification;
                            text: eventRow.modelData.verification || "";
                            color: eventRow.modelData.verification?.startsWith("Not verified") ? "#9a6700" : Theme.midGrey;
                            wrapMode: Text.Wrap;
                            Layout.fillWidth: true;
                            font.pixelSize: 12
                        }
                    }
                    ColumnLayout {
                        Layout.minimumWidth: 280;
                        Layout.maximumWidth: 280
                        spacing: 8
                        Label {
                            text: "Phát sinh: " + ui.time(eventRow.modelData.triggered_ms);
                            wrapMode: Text.Wrap;
                            Layout.fillWidth: true;
                            font.pixelSize: 13
                        }
                        Label {
                            text: "Khôi phục: " + (eventRow.modelData.recovered_ms === undefined || eventRow.modelData.recovered_ms === null ? "Chưa khôi phục" : ui.time(eventRow.modelData.recovered_ms));
                            wrapMode: Text.Wrap;
                            Layout.fillWidth: true;
                            font.pixelSize: 13;
                            color: Theme.midGrey
                        }
                    }
                }
            }
            Label {
                anchors.centerIn: parent;
                visible: events.count === 0;
                text: "Chưa có sự kiện phù hợp trong dữ liệu đã tải.";
                color: Theme.midGrey;
                width: parent.width - 40;
                wrapMode: Text.Wrap;
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
    RowLayout {
        Layout.fillWidth: true
        Label {
            text: "Active vẫn được giữ khi chưa có bằng chứng Recovery; Agent Offline có thể khiến trạng thái chưa thể xác minh.";
            color: Theme.midGrey;
            font.pixelSize: 12;
            wrapMode: Text.Wrap;
            Layout.fillWidth: true
        }
        Button {
            text: "Tải lịch sử cũ hơn";
            visible: task3Core.hasMoreEvents;
            onClicked: task3Core.loadOlderEvents()
        }
    }
}
