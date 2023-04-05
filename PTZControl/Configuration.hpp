#pragma once

#include <codecvt>
#include <map>
#include <string>
#include <fstream>
#include <toml.hpp>

class Configuration
{
public:
	void LoadFromFile(const std::wstring filename) {
		std::ifstream file{ filename, std::ios::binary | std::ios::in };
		if (!file.good()) {
			m_dirty = true;
		}
		try {
			const auto data = toml::parse(file, "configuration");
			m_reset = toml::find<bool>(data, RESET);
			m_noGuard = toml::find<bool>(data, NO_GUARD);

			if (data.contains(WINDOW_X) && data.contains(WINDOW_Y)) {
				m_hasWindowPos = true;
				m_windowPosX = toml::find<int>(data, WINDOW_X);
				m_windowPosY = toml::find<int>(data, WINDOW_Y);
			}
			if (data.contains(CAMERAS)) {
				const auto& cameras = toml::find<toml::table>(data, CAMERAS);
				for (const auto& cam : cameras) {
					try {
						const auto& tooltips = toml::find<std::vector<std::string>>(cam.second, TOOLTIPS);
						InsertTooltipsFromUtf8(cam.first, tooltips);
					}
					catch (std::out_of_range) {
					}
					try {
						const auto& name = toml::find<std::string>(cam.second, DISPLAY_NAME);
						std::wstring_convert<std::codecvt_utf8<wchar_t>> wconv;
						m_displayName[wconv.from_bytes(cam.first)] = wconv.from_bytes(name);
					}
					catch (std::out_of_range) {
					}
					try {
						const auto& time = toml::find<int>(cam.second, MOTOR_TIME);
						std::wstring_convert<std::codecvt_utf8<wchar_t>> wconv;
						m_motorTime[wconv.from_bytes(cam.first)] = time;
					}
					catch (std::out_of_range) {
					}
					try {
						const auto& control = toml::find<bool>(cam.second, LOGITECH_CONTROL);
						std::wstring_convert<std::codecvt_utf8<wchar_t>> wconv;
						m_useLogitechControl[wconv.from_bytes(cam.first)] = control;
					}
					catch (std::out_of_range) {
					}
				}
			}
		}
		catch (toml::syntax_error e) {
			m_dirty = true;
		}
		catch (std::out_of_range e) {
			m_dirty = true;
		}
		m_dirty = false;
	}

	bool WriteToFile(const std::wstring& filename) {
		if (!m_dirty) {
			return true;
		}
		const toml::value data{ {RESET, m_reset}, {NO_GUARD, m_noGuard} };
		std::ofstream file{ filename, std::ios::out | std::ios::trunc || std::ios::binary };
		if (!file.good()) {
			return false;
		}
		file << toml::format(data);
		if (m_hasWindowPos) {
			toml::value windowPos{ {WINDOW_X, m_windowPosX}, {WINDOW_Y, m_windowPosY} };
			file << toml::format(windowPos);
		}
		if (m_motorTime.size() || m_useLogitechControl.size() || m_tooltips.size()) {
			// Manually serialize the per camera settings, converting to UTF8
			std::wstring_convert<std::codecvt_utf8<wchar_t>> wconv;
			const auto camPaths = UniqueCameraPaths();
			for (const auto& cam : camPaths) {
				file << "[" << CAMERAS << "." << wconv.to_bytes(cam) << "]" << std::endl;
				if (m_tooltips.count(cam)) {
					file << TOOLTIPS << " = [" << std::endl;
					bool first = true;
					for (const auto& tip : m_tooltips[cam]) {
						file << "  \"" << wconv.to_bytes(tip) << "\"," << std::endl;
						first = false;
					}
					file << "]" << std::endl;
				}
				if (m_displayName.count(cam)) {
					file << DISPLAY_NAME << " = \"" << wconv.to_bytes(m_displayName[cam]) << "\"" << std::endl;
				}
				if (m_useLogitechControl.count(cam) && m_useLogitechControl[cam]) {
					file << LOGITECH_CONTROL << " = true" << std::endl;
				}
				if (m_motorTime.count(cam) && m_motorTime[cam] != MOTOR_TIME_DEFAULT) {
					file << MOTOR_TIME << " = " << m_motorTime[cam] << std::endl;
				}
			}
		}
		m_dirty = false;
		return true;
	}

	bool Reset() const {
		return m_reset || m_resetOver;
	}

	bool NoGuard() const {
		return m_noGuard || m_noGuardOver;
	}

	bool UseLogitechControl(const std::wstring& camPath) const {
		try {
			return m_useLogitechControl.at(camPath);
		}
		catch (std::out_of_range) {
		}
		return false;
	}

	int MotorTime(const std::wstring& camPath) const {
		try {
			return m_motorTime.at(camPath);
		}
		catch (std::out_of_range) {
		}
		return MOTOR_TIME_DEFAULT;
	}

	const std::wstring& DisplayName(const std::wstring& camPath) const {
		static const std::wstring EMPTY{};
		try {
			return m_displayName.at(camPath);
		}
		catch (std::out_of_range) {
		}
		return EMPTY;
	}

	bool ShowDevices() const {
		return m_showDevices;
	}

	int WindowPosXOrDefault(const int xDefault) const {
		if (m_hasWindowPos) {
			return m_windowPosX;
		}
		return xDefault;
	}

	int WindowPosYOrDefault(const int yDefault) const {
		if (m_hasWindowPos) {
			return m_windowPosY;
		}
		return yDefault;
	}

	const std::wstring& TooltipForPreset(const std::wstring& camPath, const size_t position) const {
		static const std::wstring EMPTY_TIP{};
		try {
			const auto& camTips = m_tooltips.at(camPath);
			if (position < camTips.size()) {
				return camTips[position];
			}
		}
		catch (std::out_of_range) {
		}
		return EMPTY_TIP;
	}

	void SetReset(bool reset) {
		if (reset != m_reset) {
			m_reset = reset;
			m_dirty = true;
		}
	}

	void SetNoGuard(bool noGuard) {
		if (noGuard != m_noGuard) {
			m_noGuard = noGuard;
			m_dirty = true;
		}
	}

	void SetUseLogitechControl(const std::wstring& camPath, bool useLogitech) {
		if (UseLogitechControl(camPath) == useLogitech) {
			return;
		}
		m_useLogitechControl[camPath] = useLogitech;
		m_dirty = true;
	}

	void SetMotorTime(const std::wstring& camPath, int motorTime) {
		if (MotorTime(camPath) == motorTime) {
			return;
		}
		m_motorTime[camPath] = motorTime;
		m_dirty = true;
	}

	void SetResetOver(bool reset) {
		m_resetOver = reset;
	}

	void SetNoGuardOver(bool noGuard) {
		m_noGuardOver = noGuard;
	}

	void SetShowDevices(bool show) {
		m_showDevices = show;
	}

	void SetWindowPos(int x, int y) {
		if (x != m_windowPosX || y != m_windowPosY) {
			m_windowPosX = x;
			m_windowPosY = y;
			m_hasWindowPos = true;
			m_dirty = true;
		}
	}

	void SetTooltipForCamera(const std::wstring& camPath, const size_t position, const std::wstring& tip) {
		if (TooltipForPreset(camPath, position) == tip) {
			return;
		}
		static const std::wstring EMPTY_TIP{};
		m_dirty = true;
		auto& camTips = m_tooltips[camPath];
		while (camTips.size() < position) {
			camTips.push_back(EMPTY_TIP);
		}
		camTips[position] = tip;
	}

	const std::vector<std::wstring>& DeviceNames() const {
		if (m_deviceName.empty() || std::find(begin(m_deviceNames), end(m_deviceNames), m_deviceName) != std::end(m_deviceNames)) {
			return m_deviceNames;
		}
		static std::vector<std::wstring> devNames;
		devNames.clear();
		devNames.reserve(m_deviceNames.size() + 1);
		std::copy(begin(m_deviceNames), end(m_deviceNames), back_inserter(devNames));
		devNames.push_back(m_deviceName);
		return devNames;
	}

private:
	static constexpr auto RESET{ "reset" };
	static constexpr auto NO_GUARD{ "no_guard" };
	static constexpr auto LOGITECH_CONTROL{ "use_logitech_control" };
	static constexpr auto MOTOR_TIME{ "motor_time" };
	static constexpr auto TOOLTIPS{ "tooltips" };
	static constexpr auto CAMERAS{ "cameras" };
	static constexpr auto DISPLAY_NAME{ "display_name" };
	static constexpr auto DEVICE_NAMES{ "device_names" };
	static constexpr auto WINDOW_X{ "window_x" };
	static constexpr auto WINDOW_Y{ "window_y" };

	static constexpr int MOTOR_TIME_DEFAULT{ 70 };
	static constexpr bool USE_LOGITECH_CONTROL_DEFAULT{ false };

	// Per camera configuration - mapped to device path
	std::map<std::wstring, std::vector<std::wstring>> m_tooltips{};
	std::map<std::wstring, bool> m_useLogitechControl{};
	std::map<std::wstring, int> m_motorTime{};
	std::map<std::wstring, std::wstring> m_displayName{};

	std::vector<std::wstring> m_deviceNames{};
	bool m_reset{ false };
	bool m_noGuard{ false };

	int m_windowPosX{ 0 };
	int m_windowPosY{ 0 };
	bool m_hasWindowPos{ false };

	// Command Line overrides/options
	bool m_resetOver{ false };
	bool m_noGuardOver{ false };
	bool m_showDevices{ false };

	std::wstring m_deviceName{};

	bool m_dirty{ false };

	void InsertTooltipsFromUtf8(const std::string& utf8CamPath, const std::vector<std::string>& utf8Tips) {
		std::wstring_convert<std::codecvt_utf8<wchar_t>> wconv;
		const auto camName = std::wstring{ wconv.from_bytes(utf8CamPath) };
		std::vector<std::wstring> tips;
		for (const auto& tip : utf8Tips) {
			tips.emplace_back(wconv.from_bytes(tip));
		}
		m_tooltips.emplace(camName, std::move(tips));
	}

	std::vector<std::wstring> UniqueCameraPaths() const {
		std::vector<std::wstring> paths;
		auto insert_unique = [&](const std::wstring& path) {
			if (std::find(begin(paths), end(paths), path) == std::end(paths)) {
				paths.push_back(path);
			}
		};
		for (const auto& dev : m_tooltips) {
			insert_unique(dev.first);
		}
		for (const auto& dev : m_useLogitechControl) {
			insert_unique(dev.first);
		}
		for (const auto& dev : m_motorTime) {
			insert_unique(dev.first);
		}
		return paths;
	}
};
