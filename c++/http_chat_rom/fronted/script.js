let websocket;
let isConnected = false;
const CHAT_SERVER_URL = 'ws://localhost:12345';

// 发送HTTP请求
function sendHttpRequest(method, path, data) {
    return fetch(`http://localhost:12345${path}`, {
        method,
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify(data)
    });
}

// 用户注册
async function registerUser() {
    const username = document.getElementById('registerUsername').value;
    const password = document.getElementById('registerPassword').value;

    const response = await sendHttpRequest('POST', '/register', {
        username,
        password
    });

    const result = await response.json();
    console.log(result);
}

// 用户登录
async function loginUser() {
    const username = document.getElementById('loginUsername').value;
    const password = document.getElementById('loginPassword').value;

    const response = await sendHttpRequest('POST', '/login', {
        username,
        password
    });

    const result = await response.json();
    if (result.success) {
        // 登录成功，跳转到聊天页面
        window.location.href = 'chat.html';
    } else {
        alert('Invalid username or password');
    }
}

// 发送消息
function sendMessage() {
    const messageInput = document.getElementById('messageInput');
    const message = messageInput.value;

    if (message.trim() === '') return;

    // 创建消息对象
    const msg = {
        type: 'group', // 消息类型，可以扩展为 'private' 等
        sender: username,
        content: message,
        timestamp: Date.now()
    };

    // 发送消息
    websocket.send(JSON.stringify(msg));
    messageInput.value = '';
}

// 接收消息
function receiveMessage(data) {
    const messageList = document.getElementById('messageList');
    const li = document.createElement('li');
    li.textContent = `[${new Date(data.timestamp)}] ${data.sender}: ${data.content}`;
    messageList.appendChild(li);
}

// WebSocket连接
function connectWebSocket() {
    websocket = new WebSocket(CHAT_SERVER_URL);

    websocket.onopen = () => {
        console.log('Connected to server');
        isConnected = true;
    };

    websocket.onmessage = (event) => {
        const data = JSON.parse(event.data);
        receiveMessage(data);
    };

    websocket.onclose = () => {
        console.log('Disconnected from server');
        isConnected = false;
        // 重新连接逻辑
        setTimeout(connectWebSocket, 3000);
    };

    websocket.onerror = (error) => {
        console.error('WebSocket error:', error);
    };
}

// 页面加载时
window.onload = () => {
    if (window.location.pathname === '/chat.html') {
        // 连接WebSocket
        connectWebSocket();
    }
};