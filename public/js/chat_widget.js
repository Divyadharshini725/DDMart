// DDMart AI Chatbot Widget (Phase 3 - Section 9 & 15)
(function() {
    function initChatWidget() {
        if (document.getElementById('ddmart-chat-widget')) return;

        // Container
        const widgetContainer = document.createElement('div');
        widgetContainer.id = 'ddmart-chat-widget';

        widgetContainer.innerHTML = `
            <!-- Launcher Button -->
            <button class="chat-launcher-btn" id="chat-launcher" title="Chat with DDMart AI Assistant" aria-label="Open AI Assistant">
                🤖
                <span class="chat-launcher-badge" id="chat-badge" style="display:none;">1</span>
            </button>

            <!-- Chat Panel -->
            <div class="chat-panel" id="chat-panel">
                <div class="chat-header">
                    <div class="chat-header-info">
                        <div class="chat-avatar">🤖</div>
                        <div>
                            <h4 class="chat-header-title">DDMart Assistant</h4>
                            <p class="chat-header-status"><span class="status-dot"></span> AI Online</p>
                        </div>
                    </div>
                    <button class="chat-close-btn" id="chat-close" title="Close chat">&times;</button>
                </div>

                <div class="chat-messages" id="chat-messages">
                    <div class="chat-msg bot">
                        👋 Hi! I am your DDMart AI Assistant. How can I help you today?
                        <div class="chat-msg-time">${formatTime()}</div>
                    </div>
                    <div class="chat-typing" id="chat-typing">
                        <span class="typing-dot"></span>
                        <span class="typing-dot"></span>
                        <span class="typing-dot"></span>
                    </div>
                </div>

                <!-- Quick Action FAQ Chips -->
                <div class="chat-suggestions" id="chat-suggestions">
                    <button class="chat-pill" data-query="What is your return policy?">↩️ Return Policy</button>
                    <button class="chat-pill" data-query="How long does shipping take?">🚚 Shipping Info</button>
                    <button class="chat-pill" data-query="What payment methods are supported?">💳 Payment Options</button>
                    <button class="chat-pill" data-query="How do I track my order?">📦 Track Orders</button>
                    <button class="chat-pill" data-query="How do I become a seller?">🏪 Become a Seller</button>
                </div>

                <div class="chat-footer">
                    <div class="chat-input-row">
                        <input type="text" class="chat-input" id="chat-input" placeholder="Ask about products, orders, returns..." maxlength="500">
                        <button class="chat-send-btn" id="chat-send" title="Send message">➤</button>
                    </div>
                    <div class="chat-char-count"><span id="chat-char-num">0</span>/500</div>
                </div>
            </div>
        `;

        document.body.appendChild(widgetContainer);

        // UI References
        const launcher = document.getElementById('chat-launcher');
        const panel = document.getElementById('chat-panel');
        const closeBtn = document.getElementById('chat-close');
        const input = document.getElementById('chat-input');
        const sendBtn = document.getElementById('chat-send');
        const messagesContainer = document.getElementById('chat-messages');
        const typingIndicator = document.getElementById('chat-typing');
        const charNum = document.getElementById('chat-char-num');
        const suggestions = document.getElementById('chat-suggestions');
        const badge = document.getElementById('chat-badge');

        function togglePanel() {
            panel.classList.toggle('active');
            if (panel.classList.contains('active')) {
                badge.style.display = 'none';
                input.focus();
                scrollToBottom();
            }
        }

        launcher.addEventListener('click', togglePanel);
        closeBtn.addEventListener('click', togglePanel);

        // Input counter
        input.addEventListener('input', () => {
            charNum.textContent = input.value.length;
        });

        // Enter key to send
        input.addEventListener('keydown', (e) => {
            if (e.key === 'Enter') {
                e.preventDefault();
                sendMessage();
            }
        });

        sendBtn.addEventListener('click', sendMessage);

        // Click on suggestion chips
        suggestions.addEventListener('click', (e) => {
            if (e.target.classList.contains('chat-pill')) {
                const query = e.target.getAttribute('data-query');
                if (query) {
                    input.value = query;
                    charNum.textContent = query.length;
                    sendMessage();
                }
            }
        });

        function formatTime() {
            const d = new Date();
            let hours = d.getHours();
            let minutes = d.getMinutes();
            if (minutes < 10) minutes = '0' + minutes;
            return `${hours}:${minutes}`;
        }

        function scrollToBottom() {
            messagesContainer.scrollTop = messagesContainer.scrollHeight;
        }

        function appendMessage(text, sender) {
            const msgDiv = document.createElement('div');
            msgDiv.className = `chat-msg ${sender}`;
            msgDiv.textContent = text;

            const timeDiv = document.createElement('div');
            timeDiv.className = 'chat-msg-time';
            timeDiv.textContent = formatTime();
            msgDiv.appendChild(timeDiv);

            messagesContainer.insertBefore(msgDiv, typingIndicator);
            scrollToBottom();
        }

        async function sendMessage() {
            const text = input.value.trim();
            if (!text) return;

            // Append user bubble
            appendMessage(text, 'user');
            input.value = '';
            charNum.textContent = '0';

            // Show typing indicator
            typingIndicator.classList.add('active');
            scrollToBottom();

            try {
                // Fetch from /api/chat with fallback to /api/v1/chat
                let res = await fetch('/api/chat', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ message: text })
                });

                if (!res.ok && res.status === 404) {
                    res = await fetch('/api/v1/chat', {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify({ message: text })
                    });
                }

                const json = await res.json();
                typingIndicator.classList.remove('active');

                let reply = '';
                if (json.reply) {
                    reply = json.reply;
                } else if (json.data && json.data.reply) {
                    reply = json.data.reply;
                } else if (json.error && json.error.message) {
                    reply = `⚠️ ${json.error.message}`;
                } else {
                    reply = "I'm sorry, I could not process your request at this time.";
                }

                appendMessage(reply, 'bot');
            } catch (err) {
                typingIndicator.classList.remove('active');
                appendMessage("I am currently having trouble reaching the server. Please check your connection or contact support@ddmart.com.", 'bot');
            }
        }
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', initChatWidget);
    } else {
        initChatWidget();
    }
})();
