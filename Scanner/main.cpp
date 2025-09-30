#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <chrono>
#include <string>
#include <vector>
#include <future>

#pragma comment(lib, "ws2_32.lib")

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable: 4996)

void clearScreen() {
    system("cls");
}

class NetworkUtils {
public:
    static bool checkTCPConnect(const std::string& host, int port, int timeoutMs = 3000) {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cout << "Ошибка инициализации WSAStartup\n";
            return false;
        }

        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET) {
            std::cout << "Ошибка создания сокета\n";
            WSACleanup();
            return false;
        }

        unsigned long timeout = timeoutMs;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));

        sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);

        if (inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr) != 1) {
            hostent* remoteHost = gethostbyname(host.c_str());
            if (remoteHost == NULL) {
                std::cout << "Ошибка разрешения имени хоста: " << host << "\n";
                closesocket(sock);
                WSACleanup();
                return false;
            }
            serverAddr.sin_addr = *(in_addr*)remoteHost->h_addr;
        }

        bool result = (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == 0);

        closesocket(sock);
        WSACleanup();

        return result;
    }

    static void tcpPing(const std::string& host, int port = 80, int count = 4) {
        std::cout << "TCP Ping " << host << " порт " << port << ":\n";

        int successCount = 0;
        for (int i = 0; i < count; i++) {
            auto start = std::chrono::steady_clock::now();
            bool success = checkTCPConnect(host, port, 2000);
            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            if (success) {
                std::cout << "OK Ответ от " << host << ": время=" << duration.count() << "мс\n";
                successCount++;
            }
            else {
                std::cout << "Таймаут\n";
            }

            if (i < count - 1) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }

        std::cout << "\nСтатистика TCP Ping для " << host << ":\n";
        std::cout << "    Пакетов: отправлено = " << count
            << ", получено = " << successCount
            << ", потеряно = " << (count - successCount)
            << " (" << ((count - successCount) * 100 / count) << "% потерь)\n";
    }
};

std::string getServiceName(int port) {
    switch (port) {
    case 21: return "FTP";
    case 22: return "SSH";
    case 23: return "Telnet";
    case 25: return "SMTP";
    case 53: return "DNS";
    case 80: return "HTTP";
    case 110: return "POP3";
    case 135: return "RPC";
    case 139: return "NetBIOS";
    case 143: return "IMAP";
    case 443: return "HTTPS";
    case 445: return "SMB";
    case 993: return "IMAPS";
    case 995: return "POP3S";
    case 1723: return "PPTP";
    case 3306: return "MySQL";
    case 3389: return "RDP";
    case 5900: return "VNC";
    case 8080: return "HTTP-Alt";
    default: return "Неизвестно";
    }
}

void portScanner() {
    clearScreen();
    std::string host;
    std::cout << "Введите хост/IP для сканирования: ";
    std::getline(std::cin, host);

    std::cout << "Сканирование основных портов...\n";

    int commonPorts[] = { 21, 22, 23, 25, 53, 80, 110, 135, 139, 143,
                         443, 445, 993, 995, 1723, 3306, 3389, 5900, 8080 };

    std::vector<std::future<bool>> futures;
    std::vector<int> openPorts;
    for (int port : commonPorts) {
        futures.push_back(std::async(std::launch::async, [host, port]() {
            return NetworkUtils::checkTCPConnect(host, port, 1000);
            }));
    }

    std::cout << "Результаты сканирования для " << host << ":\n";
    std::cout << "============================================\n";

    for (size_t i = 0; i < futures.size(); i++) {
        if (futures[i].get()) {
            std::string serviceName = getServiceName(commonPorts[i]);
            std::cout << "OK Порт " << commonPorts[i] << " (" << serviceName << ") - ОТКРЫТ\n";
            openPorts.push_back(commonPorts[i]);
        }
    }

    std::cout << "============================================\n";
    std::cout << "Найдено открытых портов: " << openPorts.size() << std::endl;
}

void tcpPingMenu() {
    clearScreen();
    std::string host;
    std::cout << "Введите хост/IP для пинга: ";
    std::getline(std::cin, host);

    int port;
    std::cout << "Введите порт для проверки (по умолчанию 80): ";
    std::string portInput;
    std::getline(std::cin, portInput);

    if (portInput.empty()) {
        port = 80;
    }
    else {
        port = std::stoi(portInput);
    }

    NetworkUtils::tcpPing(host, port);
}

void checkSpecificPort() {
    clearScreen();
    std::string host;
    int port;

    std::cout << "Введите хост/IP: ";
    std::getline(std::cin, host);

    std::cout << "Введите порт: ";
    std::cin >> port;
    std::cin.ignore();

    if (NetworkUtils::checkTCPConnect(host, port)) {
        std::cout << "OK Порт " << port << " открыт на хосте " << host << std::endl;
    }
    else {
        std::cout << "FAIL Порт " << port << " закрыт на хосте " << host << std::endl;
    }
}

void showMenu() {
    clearScreen();
    std::cout << "=== Сетевые утилиты ===\n";
    std::cout << "Использует TCP соединения\n\n";
    std::cout << "1. Сканирование портов\n";
    std::cout << "2. Пинг хоста (TCP)\n";
    std::cout << "3. Проверить конкретный порт\n";
    std::cout << "4. Выход\n";
    std::cout << "Выберите опцию: ";
}

int main() {
    setlocale(LC_ALL, "Russian");

    int choice;

    do {
        showMenu();
        std::cin >> choice;
        std::cin.ignore();

        switch (choice) {
        case 1:
            portScanner();
            break;
        case 2:
            tcpPingMenu();
            break;
        case 3:
            checkSpecificPort();
            break;
        case 4:
            std::cout << "Выход...\n";
            break;
        default:
            std::cout << "Неверный выбор!\n";
            std::cout << "\nНажмите Enter для продолжения...";
            std::cin.get();
            continue;
        }

        if (choice != 4) {
            std::cout << "\nНажмите Enter для продолжения...";
            std::cin.get();
        }

    } while (choice != 4);

    return 0;
}
