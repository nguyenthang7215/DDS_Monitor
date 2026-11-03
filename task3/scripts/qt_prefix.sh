#!/usr/bin/env bash
# Qt selection shared by the Task 3 build/run scripts (sourced, not executed).
# TASK3_QT_PREFIX wins; otherwise the newest Qt 6 gcc_64 kit that ships Qt WebEngine is used, so the
# scripts work on machines with a different Qt version than the 6.8.3 used at the office.
# TASK3_WEB_UI=OFF selects the Qt Quick dashboard and drops the WebEngine requirement.

task3_default_qt_prefix() {
    if [[ -n "${TASK3_QT_PREFIX:-}" ]]; then
        printf '%s\n' "$TASK3_QT_PREFIX"
        return
    fi
    # Newest kit wins within each rank: complete (WebEngine + Qt5Compat) > WebEngine > any Qt 6.
    local candidate complete="" web="" compat="" any=""
    while IFS= read -r candidate; do
        [[ -f "$candidate/lib/cmake/Qt6/Qt6Config.cmake" ]] || continue
        any="$candidate"
        local has_compat=no
        [[ -d "$candidate/qml/Qt5Compat/GraphicalEffects" ]] && has_compat=yes && compat="$candidate"
        if [[ -d "$candidate/lib/cmake/Qt6WebEngineQuick" ]]; then
            web="$candidate"
            [[ "$has_compat" == yes ]] && complete="$candidate"
        fi
    done < <(ls -d "$HOME"/Qt/6.*/gcc_64 /home/dev/Qt/6.*/gcc_64 /opt/Qt/6.*/gcc_64 2>/dev/null | sort -uV)
    if [[ "${TASK3_WEB_UI:-ON}" == OFF ]]; then
        printf '%s\n' "${compat:-${any:-/home/dev/Qt/6.8.3/gcc_64}}"
    else
        printf '%s\n' "${complete:-${web:-${any:-/home/dev/Qt/6.8.3/gcc_64}}}"
    fi
}

# Prints every missing piece of the Qt kit (build and runtime) and returns 1 if anything is missing.
task3_check_qt_modules() {
    local prefix="$1" missing=() module
    [[ -f "$prefix/lib/cmake/Qt6/Qt6Config.cmake" ]] || missing+=("Qt 6 itself ($prefix/lib/cmake/Qt6)")
    for module in Core Network Sql Quick QuickControls2 Concurrent Charts Widgets Test; do
        [[ -d "$prefix/lib/cmake/Qt6$module" ]] || missing+=("Qt6$module")
    done
    # MainStartView.qml of the original DDS Monitor imports Qt5Compat.GraphicalEffects at runtime.
    [[ -d "$prefix/qml/Qt5Compat/GraphicalEffects" ]] || missing+=("Qt 5 Compatibility Module (qml/Qt5Compat/GraphicalEffects)")
    if [[ "${TASK3_WEB_UI:-ON}" != OFF ]]; then
        for module in WebEngineQuick WebChannelQuick; do
            [[ -d "$prefix/lib/cmake/Qt6$module" ]] || missing+=("Qt6$module (Qt WebEngine / Qt WebChannel)")
        done
        [[ -x "$prefix/libexec/QtWebEngineProcess" ]] || missing+=("QtWebEngineProcess ($prefix/libexec)")
    fi
    if ((${#missing[@]})); then
        printf 'Qt kit %s is missing:\n' "$prefix" >&2
        printf '  - %s\n' "${missing[@]}" >&2
        printf 'Install them for this Qt version with the Qt MaintenanceTool, choose another kit with\n' >&2
        printf 'TASK3_QT_PREFIX=<qt>/gcc_64, or build the Qt Quick dashboard with TASK3_WEB_UI=OFF.\n' >&2
        return 1
    fi
}
