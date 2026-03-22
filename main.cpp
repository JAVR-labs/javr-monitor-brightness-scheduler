#include <iostream>
#include <string_view>
#include <vector>
#include <optional>

#include <fstream>
#include <QProcess>
#include <QString>
#include <QDateTime>
#include <QThread>

#include <filesystem>
#include <unistd.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

constexpr std::string_view SETTINGS_FILE = "settings.json";
constexpr std::string_view MONITORS = "monitors";
constexpr std::string_view TIMESTAMPS = "timestamps";
constexpr std::string_view TIMESTAMP_HOUR = "hour";
constexpr std::string_view TIMESTAMP_BRIGHTNESS = "brightness";
constexpr int PROCESS_TIMEOUT = 5000;

using optVecOfStr = std::optional<std::vector<std::string> >;

std::filesystem::path getExePath() {
    char path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
    return {std::string(path, (count > 0) ? count : 0)};
}

void handleProcessEnd(QProcess &process) {
    if (!process.waitForFinished(PROCESS_TIMEOUT)) {
        std::cerr << "Force termination of child process\n";
        process.terminate();
        if (!process.waitForFinished(PROCESS_TIMEOUT)) {
            std::cerr << "Force kill of child process\n";
            process.kill();
            process.waitForFinished();
        }
    }
}

std::optional<int> getMonitorBrightness(const std::string &monitor) {
    const QString awkScript = QString(
        "/Output:/ { found = ($0 ~ mon) } "
        "found && /Brightness control/ { match($0, /set to ([0-9]+)/, a); print a[1]; found=0 }"
    );

    QProcess kscreen;
    QProcess awk;

    kscreen.setStandardOutputProcess(&awk);

    awk.start(
        "nice",
        QStringList() << "-n" << "19"
        << "awk" << "-v" << QString("mon=%1").arg(monitor) << awkScript
    );
    kscreen.start(
        "nice",
        QStringList() << "-n" << "19"
        << "kscreen-doctor" << "-o"
    );

    handleProcessEnd(kscreen);
    handleProcessEnd(awk);
    QString tmp = QString::fromUtf8(awk.readAllStandardOutput()).trimmed();
    bool ok;
    int result = tmp.toInt(&ok);
    return ok ? std::optional<int>(result) : std::nullopt;
}

using optVecOfStr = std::optional<std::vector<std::string> >;

optVecOfStr getMonitorsNames(const json &monitorsJson) {
    optVecOfStr result = std::vector<std::string>{};;
    result->reserve(monitorsJson.size());
    for (const auto &monitor: monitorsJson) {
        try {
            result->push_back(monitor.get<std::string>());
        } catch (const std::exception &e) {
            std::cerr << "Failed to load monitors array - " << e.what() << std::endl;
            return std::nullopt;
        }
    }
    return result;
}

int setBrightness(const std::vector<std::string> &monitors, std::vector<int> values) {
    if (values.size() != monitors.size()) {
        std::cerr << "Wrong length of brightness array" << std::endl;
        return 6;
    }

    for (auto &value: values) {
        value = (value > 100) ? 100 : (value < 0) ? 0 : value;
    }

    std::vector<int> currentBrightness;
    for (size_t i = 0; i < monitors.size(); i++) {
        currentBrightness.push_back(
            getMonitorBrightness(monitors[i]).value_or(values[i])
        );
        std::cout << "Setting monitor " << monitors[i] << " brightness to: " << values[i] << "%" << '\n';
    }

    bool isDifferentBrightness;
    do {
        isDifferentBrightness = false;
        for (size_t i = 0; i < monitors.size(); i++) {
            if (values[i] > currentBrightness[i]) {
                if (values[i] > currentBrightness[i] + 10) {
                    currentBrightness[i]+= 10;
                } else {
                    currentBrightness[i]++;
                }
                isDifferentBrightness = true;
            } else if (values[i] < currentBrightness[i]) {
                if (values[i] < currentBrightness[i] - 10) {
                    currentBrightness[i]-= 10;
                } else {
                    currentBrightness[i]--;
                }
                isDifferentBrightness = true;
            }
            QStringList kscreenArgs = {
                QString("output.%1.brightness.%2").arg(monitors[i]).arg(currentBrightness[i])
            };
            QProcess::execute("kscreen-doctor", kscreenArgs);
        }
        sleep(1);
    } while (isDifferentBrightness);
    return 0;
}

void setBrightness(const std::vector<std::string> &monitors, int value) {
    value = (value > 100) ? 100 : (value < 0) ? 0 : value;
    std::vector<int> values;
    for (size_t i = 0; i < monitors.size(); i++) {
        values.push_back(value);
    }
    setBrightness(monitors, values);
}

struct BrightnessData {
    explicit BrightnessData(const json &jsonData) {
        if (jsonData.is_array()) {
            vectorValue = std::vector<int>{};
            for (const auto &value: jsonData) {
                try {
                    vectorValue->push_back(value.get<int>());
                } catch (const std::exception &e) {
                    std::cerr << "Failed to load timestamp brightness array - " << e.what() << std::endl;
                    vectorValue.reset();
                    break;
                }
            }
        } else {
            try {
                intValue.emplace(jsonData.get<int>());
            } catch (const std::exception &e) {
                std::cerr << "Failed to load timestamp brightness value - " << e.what() << std::endl;
                intValue.reset();
            }
        }
    }

    std::optional<std::vector<int> > vectorValue;
    std::optional<int> intValue;
};

int autoSet(const std::vector<std::string> &monitors, const json &timestamps) {
    const int currentHour = QDateTime::currentDateTime().time().hour();

    uint8_t index = 0;
    int hour = 0;
    for (const auto &timestamp: timestamps) {
        try {
            hour = timestamp[TIMESTAMP_HOUR].get<int>();
        } catch (const std::exception &e) {
            std::cerr << "Failed to load timestamp hour - " << e.what() << '\n';
            return 7;
        }

        if (currentHour < hour) {
            break;
        }
        index++;
    }
    if (index != 0) index--;

    const BrightnessData brightnessData(timestamps[index][TIMESTAMP_BRIGHTNESS]);
    if (brightnessData.intValue.has_value()) {
        setBrightness(monitors, brightnessData.intValue.value());
        return 0;
    }
    if (brightnessData.vectorValue.has_value()) {
        setBrightness(monitors, brightnessData.vectorValue.value());
        return 0;
    }

    return 8;
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
        return 2;
    }

    // read monitors data
    optVecOfStr monitors = getMonitorsNames(data[MONITORS]);
    if (!monitors.has_value()) return 3;

    for (const auto &monitor: monitors.value()) {
        std::cout << "Jej " << monitor << ": " << getMonitorBrightness(monitor).value_or(-1) << "\n";
    }

    // read timestamps data
    const json &timestamps = data[TIMESTAMPS];

    if (argc < 2) {
        return autoSet(monitors.value(), timestamps);
    }

    if (argc == 2) {
        bool ok;
        const int value = QString(argv[1]).toInt(&ok);
        if (!ok) {
            std::cerr << "Arguments must be integers" << '\n';
            return 4;
        }
        setBrightness(monitors.value(), value);
    } else {
        if (argc - 1 != monitors.value().size()) {
            std::cerr << "Wrong number of arguments" << std::endl;
            return 6;
        }
        bool ok;
        std::vector<int> values;
        values.reserve(monitors.value().size());
        for (uint8_t i = 1; i < argc; i++) {
            int tmp = QString(argv[i]).toInt(&ok);
            if (!ok) {
                std::cerr << "Arguments must be integers" << '\n';
                return 4;
            }
            values.push_back(tmp);
        }
        setBrightness(monitors.value(), values);
    }

    return 0;
}
