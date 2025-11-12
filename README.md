# 📁 P2P File Transfer (C++)

This project is a **peer-to-peer (P2P) file transfer program** made using **C++ and Winsock (Windows Sockets)**.  
It allows two computers to **send and receive files** directly over a network — without any external server.

---

## ⚙️ How It Works

The program runs in two modes:

1. **Server (Receiver)** – Listens on a port and receives files.  
2. **Client (Sender/Downloader)** – Connects to another computer to send or download a file.

---

## 🧠 Features

- Works on **Windows** using Winsock2  
- Simple command-line usage  
- Supports both upload (`put`) and download (`get`)  
- Handles large files efficiently  
- No external dependencies

---

## 🛠️ Build Instructions

1. Open **Command Prompt**  
2. Compile using `g++` (MinGW or any C++11 compiler):

   ```bash
   g++ -std=c++11 p2p_final.cpp -lws2_32 -o p2p.exe
🚀 Usage
1️⃣ Start the Server (Receiver)

Run this on the computer that will receive files:

p2p.exe listen 8080


✅ The server will start listening on port 8080.

2️⃣ Send a File (Uploader)

Run this on the computer that will send the file:

p2p.exe put <receiver_ip> <port> <file_path> [remote_name]


Example:

p2p.exe put 192.168.1.10 8080 test.txt


✅ Sends test.txt to the receiver.

3️⃣ Download a File (Downloader)

Run this on the computer that wants to download a file from the receiver:

p2p.exe get <receiver_ip> <port> <file_name> [save_as]


Example:

p2p.exe get 192.168.1.10 8080 report.pdf


✅ Downloads report.pdf from the remote system.

📦 Example Workflow

On PC 1 (Receiver):

p2p.exe listen 8080


On PC 2 (Sender):

p2p.exe put 192.168.1.5 8080 myfile.txt


🎉 PC 1 will receive the file myfile.txt.

🧾 Notes

Works only on Windows

Both systems must be on the same network

Make sure your firewall allows the port you’re using (e.g., 8080)

Avoid using unsafe filenames (like ..\file.txt)
