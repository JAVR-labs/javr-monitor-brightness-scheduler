#include <iostream>
#include <string_view>

#include <fstream>
#include <QProcess>
#include <QString>
#include <QDateTime>

#include <filesystem>
#include <unistd.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

constexpr std::string_view SETTINGS_FILE = "settings.json";
constexpr std::string_view MONITORS = "monitors";
constexpr std::string_view TIMESTAMPS = "timestamps";
constexpr std::string_view TIMESTAMP_HOUR = "hour";
constexpr std::string_view TIMESTAMP_BRIGHTNESS = "brightness";

std::filesystem::path getExePath() {
    char path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
    return {std::string(path, (count > 0) ? count : 0)};
}

void setBrightness(const json & monitors, int value) {
    value = value > 100 ? 100 : value < 0 ? 0 : value;

    for (const auto &monitor: monitors) {
        std::cout << "Setting monitor " << monitor << " brightness to: " << value << "%" << '\n';
        QStringList kscreenArgs = {
            QString("output.%1.brightness.%2").arg(monitor.get<std::string>()).arg(value)
        };
        QProcess::execute("kscreen-doctor", kscreenArgs);
    }
}

int autoSet(const json &monitors, const json &timestamps) {
    const int currentHour = QDateTime::currentDateTime().time().hour();

    uint8_t index = 0;
    int hour = 0;
    for (const auto &timestamp: timestamps) {
        try {
            hour = timestamp[TIMESTAMP_HOUR].get<int>();
        } catch (const std::exception &e) {
            std::cerr << "Failed to load timestamp hour - " << e.what() << '\n';
            return 1;
        }

        if (currentHour < hour) {
            break;
        }
        index++;
    }

    if (index != 0) index--;

    int brightness;
    try {
        brightness = timestamps[index][TIMESTAMP_BRIGHTNESS].get<int>();
    } catch (const std::exception &e) {
        std::cerr << "Failed to load timestamp brightness - " << e.what() << '\n';
        return 1;
    }

    setBrightness(monitors, brightness);
    return 0;
}

int main(const int argc, char *argv[]) {
    std::string filePath = getExePath().parent_path().string() + "/" + SETTINGS_FILE.data();
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Unable to open settings.json" << '\n';
        return 1;
    }

    const json data = json::parse(file);
    if (!data.contains(MONITORS) || !data.contains(TIMESTAMPS)) {
        std::cerr << "Invalid settings.json" << '\n';
        return 1;
    }

    const json &monitors = data[MONITORS];
    const json &timestamps = data[TIMESTAMPS];

    if (argc < 2) {
        return autoSet(monitors, timestamps);
    }

    bool ok;
    const int value = QString(argv[1]).toInt(&ok);
    if (!ok) {
        std::cerr << "Argument must be an integer." << '\n';
        return 1;
    }

    setBrightness(monitors, value);
    return 0;
}
