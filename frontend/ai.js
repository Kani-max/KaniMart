/* =========================================================
   KaMa AI
   KaniMart AI Assistant
   ========================================================= */

const API_BASE = "http://127.0.0.1:8080";

const TOKEN_KEY = "kanimart_token";
const USER_KEY = "kanimart_user";


/* =========================================================
   DOM ELEMENTS
   ========================================================= */

const chatForm = document.getElementById("chatForm");
const chatInput = document.getElementById("chatInput");
const sendButton = document.getElementById("sendButton");
const chatMessages = document.getElementById("chatMessages");
const chatWelcome = document.getElementById("chatWelcome");


/* =========================================================
   SESSION
   ========================================================= */

function getToken() {
    return localStorage.getItem(TOKEN_KEY);
}

function getUser() {
    try {
        const user = localStorage.getItem(USER_KEY);
        return user ? JSON.parse(user) : null;
    } catch (error) {
        console.error("Unable to read saved user:", error);
        return null;
    }
}


/* =========================================================
   HTML ESCAPING
   ========================================================= */

function escapeHtml(value) {
    if (value === null || value === undefined) {
        return "";
    }

    return String(value)
        .replace(/&/g, "&amp;")
        .replace(/</g, "&lt;")
        .replace(/>/g, "&gt;")
        .replace(/"/g, "&quot;")
        .replace(/'/g, "&#039;");
}


/* =========================================================
   MESSAGE TEXT
   ========================================================= */

function getAssistantText(result) {

    if (result === null || result === undefined) {
        return "I couldn't generate a response.";
    }

    if (typeof result === "string") {
        return result;
    }

    /*
     * Support several common backend response shapes.
     *
     * Example:
     * {
     *     "message": "Hello"
     * }
     *
     * or:
     *
     * {
     *     "data": {
     *         "message": "Hello"
     *     }
     * }
     */

    if (typeof result.message === "string") {
        return result.message;
    }

    if (typeof result.response === "string") {
        return result.response;
    }

    if (typeof result.reply === "string") {
        return result.reply;
    }

    if (typeof result.answer === "string") {
        return result.answer;
    }

    if (result.data) {

        if (typeof result.data === "string") {
            return result.data;
        }

        if (typeof result.data.message === "string") {
            return result.data.message;
        }

        if (typeof result.data.response === "string") {
            return result.data.response;
        }

        if (typeof result.data.reply === "string") {
            return result.data.reply;
        }

        if (typeof result.data.answer === "string") {
            return result.data.answer;
        }
    }

    return "I received a response, but I couldn't understand its format.";
}


/* =========================================================
   ADD USER MESSAGE
   ========================================================= */

function addUserMessage(message) {

    if (chatWelcome) {
        chatWelcome.remove();
    }

    const row = document.createElement("div");
    row.className = "message-row user";

    row.innerHTML = `
        <div class="chat-message">
            <span class="message-name">YOU</span>
            ${escapeHtml(message)}
        </div>
    `;

    chatMessages.appendChild(row);

    scrollToBottom();
}


/* =========================================================
   ADD AI MESSAGE
   ========================================================= */

function addAssistantMessage(message) {

    if (chatWelcome) {
        chatWelcome.remove();
    }

    const row = document.createElement("div");
    row.className = "message-row assistant";

    row.innerHTML = `
        <div class="chat-message">
            <span class="message-name">KaMa AI</span>
            ${escapeHtml(message)}
        </div>
    `;

    chatMessages.appendChild(row);

    scrollToBottom();
}


/* =========================================================
   TYPING INDICATOR
   ========================================================= */

function showTypingIndicator() {

    removeTypingIndicator();

    const row = document.createElement("div");

    row.className = "message-row assistant";
    row.id = "typingIndicator";

    row.innerHTML = `
        <div class="typing">
            <span></span>
            <span></span>
            <span></span>
        </div>
    `;

    chatMessages.appendChild(row);

    scrollToBottom();
}


function removeTypingIndicator() {

    const existing =
        document.getElementById("typingIndicator");

    if (existing) {
        existing.remove();
    }
}


/* =========================================================
   SCROLL CHAT
   ========================================================= */

function scrollToBottom() {

    requestAnimationFrame(() => {
        chatMessages.scrollTop =
            chatMessages.scrollHeight;
    });
}


/* =========================================================
   SEND MESSAGE
   ========================================================= */

async function sendMessage(message) {

    const cleanMessage = message.trim();

    if (!cleanMessage) {
        return;
    }

    addUserMessage(cleanMessage);

    chatInput.value = "";
    autoResizeInput();

    sendButton.disabled = true;
    chatInput.disabled = true;

    showTypingIndicator();

    try {

        const token = getToken();
        const user = getUser();

        const headers = {
            "Content-Type": "application/json"
        };

        /*
         * If the user is logged in, send the JWT.
         * This keeps KaMa AI compatible with your
         * existing authenticated KaniMart application.
         */

        if (token) {
            headers["Authorization"] =
                `Bearer ${token}`;
        }

        const requestBody = {
            message: cleanMessage
        };

        /*
         * Include user information when available.
         * The backend can ignore these fields if it
         * does not need them.
         */

        if (user && user.id) {
            requestBody.user_id = user.id;
        }

        const response = await fetch(
            `${API_BASE}/api/chat`,
            {
                method: "POST",
                headers,
                body: JSON.stringify(requestBody)
            }
        );

        const rawText = await response.text();

        let result = null;

        try {
            result = rawText
                ? JSON.parse(rawText)
                : null;
        } catch (parseError) {
            result = rawText;
        }

        removeTypingIndicator();

        if (!response.ok) {

            let errorMessage =
                "KaMa AI could not process your request.";

            if (result) {

                if (typeof result === "string") {
                    errorMessage = result;
                }
                else if (result.message) {
                    errorMessage = result.message;
                }
                else if (result.error) {
                    errorMessage = result.error;
                }
                else if (result.data?.message) {
                    errorMessage = result.data.message;
                }
            }

            addAssistantMessage(errorMessage);

            return;
        }

        const assistantMessage =
            getAssistantText(result);

        addAssistantMessage(assistantMessage);

    }
    catch (error) {

        console.error(
            "KaMa AI request failed:",
            error
        );

        removeTypingIndicator();

        addAssistantMessage(
            "I couldn't connect to the KaniMart server. " +
            "Please make sure the backend is running and try again."
        );

    }
    finally {

        sendButton.disabled = false;
        chatInput.disabled = false;

        chatInput.focus();
    }
}


/* =========================================================
   FORM SUBMIT
   ========================================================= */

chatForm.addEventListener(
    "submit",
    async function(event) {

        event.preventDefault();

        const message = chatInput.value.trim();

        if (!message) {
            return;
        }

        await sendMessage(message);
    }
);


/* =========================================================
   ENTER TO SEND
   ========================================================= */

chatInput.addEventListener(
    "keydown",
    function(event) {

        /*
         * Enter = send
         * Shift + Enter = new line
         */

        if (
            event.key === "Enter" &&
            !event.shiftKey
        ) {

            event.preventDefault();

            chatForm.requestSubmit();
        }
    }
);


/* =========================================================
   AUTO RESIZE TEXTAREA
   ========================================================= */

function autoResizeInput() {

    chatInput.style.height = "auto";

    const newHeight =
        Math.min(chatInput.scrollHeight, 120);

    chatInput.style.height =
        `${newHeight}px`;
}


chatInput.addEventListener(
    "input",
    autoResizeInput
);


/* =========================================================
   SUGGESTION BUTTONS
   ========================================================= */

const suggestionButtons =
    document.querySelectorAll(
        ".suggestion-btn"
    );

suggestionButtons.forEach(
    button => {

        button.addEventListener(
            "click",
            async function() {

                const message =
                    button.dataset.message;

                if (!message) {
                    return;
                }

                chatInput.value = message;

                autoResizeInput();

                await sendMessage(message);
            }
        );

    }
);


/* =========================================================
   INITIALIZATION
   ========================================================= */

function initializeKaMaAI() {

    chatInput.focus();

    autoResizeInput();

    /*
     * The AI page can be opened from the home page
     * without being logged in. If the backend requires
     * authentication, sendMessage() automatically uses
     * the saved JWT.
     */

    console.log("KaMa AI initialized.");
}


initializeKaMaAI();