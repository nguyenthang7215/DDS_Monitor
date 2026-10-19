// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Theme 1.0

ColumnLayout {
    id: view
    required property var ui
    readonly property var rows: task3Core.flows.filter(f => (f.source_host === ui.selectedHostId || f.target_host === ui.selectedHostId) && (!appScope.checked || !ui.selectedApplicationId || (f.source_host === ui.selectedHostId && f.source_application === ui.selectedApplicationId) || (f.target_host === ui.selectedHostId && f.target_application === ui.selectedApplicationId)))
    readonly property var selected: rows.find(f => ui.flowKey(f) === ui.selectedFlowKey) || ({})
    spacing: 12
    RowLayout {
        Layout.fillWidth: true
        CheckBox {
            id: appScope
            text: ui.selectedApplicationId ? "Ứng dụng: " + ui.selectedApplicationId : "Tất cả ứng dụng trên Slave"
            checked: true;
            enabled: !!ui.selectedApplicationId;
            Layout.fillWidth: true;
            Layout.minimumWidth: 0
            contentItem: Label {
                text: appScope.text;
                leftPadding: appScope.indicator.width + appScope.spacing;
                verticalAlignment: Text.AlignVCenter;
                elide: Text.ElideRight
            }
            ToolTip.visible: hovered;
            ToolTip.text: text
        }
        Button {
            text: "DDS chuyên sâu";
            onClicked: ui.openDdsWorkspace(view.selected.writer_id || "")
        }
    }
    Label {
        text: "Discovery, matching và hoạt động dữ liệu là các bằng chứng riêng. Chỉ vẽ mũi tên khi matching được xác nhận.";
        wrapMode: Text.Wrap;
        Layout.fillWidth: true;
        color: Theme.midGrey;
        font.pixelSize: 12
    }
    SplitView {
        Layout.fillWidth: true;
        Layout.fillHeight: true
        orientation: Qt.Vertical
        ListView {
            id: flows
            SplitView.fillHeight: true
            SplitView.minimumHeight: 110
            clip: true;
            spacing: 4
            model: view.rows
            ScrollBar.vertical: ScrollBar {}
            delegate: ItemDelegate {
                id: flowButton
                required property var modelData
                width: flows.width;
                height: 84
                highlighted: ui.flowKey(modelData) === ui.selectedFlowKey
                background: Rectangle {
                    radius: 4;
                    color: flowButton.highlighted ? "#eaf2f9" : flowButton.hovered ? "#f3f6f9" : "white";
                    border.color: flowButton.highlighted || flowButton.activeFocus ? Theme.eProsimaDarkBlue : Theme.lightGrey
                }
                ToolTip.visible: hovered
                ToolTip.text: (modelData.topic || "Unknown Topic") + "\n" + (modelData.source_application || "Unmapped app") + " → " + (modelData.target_application || "Unmapped app")
                onClicked: ui.selectedFlowKey = ui.flowKey(modelData)
                contentItem: RowLayout {
                    spacing: 12
                    ColumnLayout {
                        Layout.fillWidth: true
                        Label {
                            text: modelData.topic || "Unknown Topic";
                            font.bold: true;
                            Layout.fillWidth: true;
                            elide: Text.ElideRight
                        }
                        Label {
                            text: (modelData.source_application || "Unmapped app") + (modelData.confirmed ? " → " : " ··· ") + (modelData.target_application || "Unmapped app");
                            Layout.fillWidth: true;
                            elide: Text.ElideRight;
                            font.pixelSize: 12
                        }
                        Label {
                            text: "Writer #" + modelData.writer_id + " / Reader #" + (modelData.reader_id || "Unknown");
                            color: Theme.midGrey;
                            font.pixelSize: 12
                        }
                    }
                    ColumnLayout {
                        Layout.minimumWidth: 170;
                        Layout.maximumWidth: 210
                        Label {
                            text: modelData.match_state || "Unknown";
                            font.bold: true;
                            color: modelData.confirmed ? "#237a45" : Theme.midGrey
                        }
                        Label {
                            text: modelData.flow_state || "Unknown";
                            Layout.fillWidth: true;
                            elide: Text.ElideRight;
                            font.pixelSize: 12
                        }
                    }
                }
            }
            Label {
                anchors.centerIn: parent;
                width: parent.width - 32;
                wrapMode: Text.Wrap;
                horizontalAlignment: Text.AlignHCenter;
                visible: flows.count === 0;
                text: "Chưa có luồng được ánh xạ trong phạm vi đã chọn. Xem thực thể Unmapped tại DDS chuyên sâu.";
                color: Theme.midGrey
            }
        }
        Frame {
            SplitView.preferredHeight: 280
            SplitView.minimumHeight: 140
            padding: 14
            background: Rectangle {
                color: "white";
                border.color: Theme.lightGrey;
                radius: 4
            }
            ScrollView {
                id: inspector
                anchors.fill: parent
                clip: true
                contentWidth: availableWidth
                ColumnLayout {
                    width: inspector.availableWidth
                    spacing: 12
                    Label {
                        text: view.selected.writer_id ? "Chi tiết luồng DDS" : "Chọn một luồng để xem chi tiết";
                        font.bold: true;
                        color: Theme.eProsimaDarkBlue;
                        font.pixelSize: 17
                    }
                    RowLayout {
                        visible: !!view.selected.writer_id
                        Layout.fillWidth: true
                        spacing: 12
                        Label {
                            text: (view.selected.source_host || "Unmapped host") + " / " + (view.selected.source_application || "Unmapped app") + "\nWriter #" + (view.selected.writer_id || "—");
                            Layout.fillWidth: true;
                            Layout.preferredWidth: 1;
                            Layout.minimumWidth: 0;
                            wrapMode: Text.Wrap
                        }
                        Label {
                            text: view.selected.confirmed ? "→" : "···";
                            font.pixelSize: 22;
                            color: Theme.eProsimaLightBlue
                        }
                        Label {
                            text: view.selected.topic || "Unknown Topic";
                            Layout.fillWidth: true;
                            Layout.preferredWidth: 1;
                            Layout.minimumWidth: 0;
                            wrapMode: Text.Wrap;
                            font.bold: true;
                            color: Theme.eProsimaDarkBlue
                        }
                        Label {
                            text: view.selected.confirmed ? "→" : "···";
                            font.pixelSize: 22;
                            color: Theme.eProsimaLightBlue
                        }
                        Label {
                            text: (view.selected.target_host || "Unmapped host") + " / " + (view.selected.target_application || "Unmapped app") + "\nReader #" + (view.selected.reader_id || "Unknown");
                            Layout.fillWidth: true;
                            Layout.preferredWidth: 1;
                            Layout.minimumWidth: 0;
                            wrapMode: Text.Wrap
                        }
                    }
                    Label {
                        visible: !!view.selected.writer_id;
                        text: "Matching: " + (view.selected.match_state || "Unknown") + " · Data: " + (view.selected.flow_state || "Unknown");
                        wrapMode: Text.Wrap;
                        Layout.fillWidth: true
                    }
                    RowLayout {
                        visible: !!view.selected.writer_id
                        Layout.fillWidth: true
                        spacing: 24
                        ColumnLayout {
                            Layout.fillWidth: true
                            Label {
                                text: "THROUGHPUT · WRITER";
                                color: Theme.midGrey;
                                font.pixelSize: 12
                            }
                            Label {
                                text: ui.number(view.selected.throughput_bps) + " B/s";
                                font.bold: true;
                                font.pixelSize: 22;
                                color: Theme.eProsimaDarkBlue
                            }
                            Label {
                                text: "Cập nhật: " + ui.time(view.selected.throughput_timestamp_ms);
                                font.pixelSize: 13;
                                wrapMode: Text.Wrap;
                                Layout.fillWidth: true
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            Label {
                                text: "LATENCY · WRITER / READER";
                                color: Theme.midGrey;
                                font.pixelSize: 12
                            }
                            Label {
                                text: ui.number(view.selected.latency_us) + " µs";
                                font.bold: true;
                                font.pixelSize: 22;
                                color: Theme.eProsimaDarkBlue
                            }
                            Label {
                                text: "Cập nhật: " + ui.time(view.selected.latency_timestamp_ms);
                                font.pixelSize: 13;
                                wrapMode: Text.Wrap;
                                Layout.fillWidth: true
                            }
                        }
                    }
                    Label {
                        visible: !!view.selected.writer_id;
                        text: "Thống kê theo cửa sổ 120 s hiện có. Biểu đồ động/lịch sử và QoS nằm trong DDS chuyên sâu.";
                        color: Theme.midGrey;
                        wrapMode: Text.Wrap;
                        Layout.fillWidth: true;
                        font.pixelSize: 12
                    }
                }
            }
        }
    }
}
