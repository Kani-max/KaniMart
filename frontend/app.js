const API_BASE = "http://127.0.0.1:8080";

const authSection = document.getElementById("authSection");
const marketplaceSection = document.getElementById("marketplaceSection");
const loginForm = document.getElementById("loginForm");
const message = document.getElementById("message");
const welcomeUser = document.getElementById("welcomeUser");
const logoutButton = document.getElementById("logoutButton");
const searchInput = document.getElementById("searchInput");
const categoryFilter = document.getElementById("categoryFilter");
const refreshProducts = document.getElementById("refreshProducts");
const productGrid = document.getElementById("productGrid");
const productCount = document.getElementById("productCount");
const productMessage = document.getElementById("productMessage");
const cartButton = document.getElementById("cartButton");
const cartCount = document.getElementById("cartCount");
const checkoutButton = document.getElementById("checkoutButton");
const orderHistoryButton = document.getElementById("orderHistoryButton");

let products = [];

let cart = {
    items: [],
    total_cents: 0
};


function getToken() {
    return localStorage.getItem("kanimart_token");
}


function getUser() {
    const value = localStorage.getItem("kanimart_user");

    if (!value) {
        return null;
    }

    try {
        return JSON.parse(value);
    } catch {
        return null;
    }
}


function authHeaders() {
    return {
        "Content-Type": "application/json",
        "Authorization": `Bearer ${getToken()}`
    };
}


function updateCartCount() {
    const count = cart.items.reduce(
        (total, item) => total + Number(item.quantity || 0),
        0
    );

    cartCount.textContent = count;
}


function showMarketplace() {
    const user = getUser();

    if (!user || !getToken()) {
        authSection.classList.remove("hidden");
        marketplaceSection.classList.add("hidden");
        welcomeUser.textContent = "";
        return;
    }

    authSection.classList.add("hidden");
    marketplaceSection.classList.remove("hidden");

    welcomeUser.textContent =
        `${user.name} (${user.role})`;

    updateCartCount();

    loadProducts();
    loadCart();
}


async function loadProducts() {
    productMessage.textContent = "Loading products...";
    productGrid.innerHTML = "";

    try {
        const response = await fetch(
            `${API_BASE}/api/products`
        );

        const result = await response.json();

        if (!response.ok) {
            throw new Error(
                result.error || "Unable to load products."
            );
        }

        products = Array.isArray(result)
            ? result
            : (result.data || []);

        populateCategories();
        renderProducts();

    } catch (error) {
        console.error(error);

        productCount.textContent = "0 products";

        productMessage.textContent =
            "Unable to load products from KaniMart.";
    }
}


async function loadCart() {
    const user = getUser();

    if (!user) {
        return;
    }

    try {
        const response = await fetch(
            `${API_BASE}/api/cart/${user.id}`,
            {
                method: "GET",
                headers: authHeaders()
            }
        );

        const result = await response.json();

        if (!response.ok || !result.success) {
            throw new Error(
                result.error || "Unable to load cart."
            );
        }

        cart = result.data || {
            items: [],
            total_cents: 0
        };

        updateCartCount();

    } catch (error) {
        console.error(error);

        cart = {
            items: [],
            total_cents: 0
        };

        updateCartCount();
    }
}


function populateCategories() {
    const currentCategory = categoryFilter.value;

    const categories = [
        ...new Set(
            products
                .map(product => product.category)
                .filter(Boolean)
        )
    ].sort();

    categoryFilter.innerHTML =
        '<option value="">All categories</option>';

    categories.forEach(category => {
        const option = document.createElement("option");

        option.value = category;
        option.textContent = category;

        categoryFilter.appendChild(option);
    });

    categoryFilter.value = currentCategory;
}


function getFilteredProducts() {
    const search = searchInput.value
        .trim()
        .toLowerCase();

    const category = categoryFilter.value;

    return products.filter(product => {
        const matchesSearch =
            !search ||
            String(product.name || "")
                .toLowerCase()
                .includes(search) ||
            String(product.description || "")
                .toLowerCase()
                .includes(search);

        const matchesCategory =
            !category ||
            product.category === category;

        return matchesSearch && matchesCategory;
    });
}
async function loadReviews(productId) {
    const reviewContainer =
        document.getElementById(`reviews-${productId}`);

    if (!reviewContainer) {
        return;
    }

    try {
        const response = await fetch(
            `${API_BASE}/api/products/${productId}/reviews`
        );

        const result = await response.json();

        if (!response.ok || !result.success) {
            reviewContainer.innerHTML =
                "<p>No reviews available.</p>";
            return;
        }

        const reviews = result.data?.reviews || [];

        if (reviews.length === 0) {
            reviewContainer.innerHTML =
                "<p>No reviews yet.</p>";
            return;
        }

        reviewContainer.innerHTML = reviews
            .map(review => {
                const rating =
                    Number(review.rating || 0);

                const stars =
                    "★".repeat(rating) +
                    "☆".repeat(5 - rating);

                return `
                    <div class="review-item">
                        <strong>${stars}</strong>
                        <p>${escapeHtml(
                            review.comment || ""
                        )}</p>
                    </div>
                `;
            })
            .join("");

    } catch (error) {
        console.error(error);

        reviewContainer.innerHTML =
            "<p>Unable to load reviews.</p>";
    }
}

function renderProducts() {
    const filteredProducts = getFilteredProducts();

    productCount.textContent =
        `${filteredProducts.length} product${filteredProducts.length === 1 ? "" : "s"}`;

    productMessage.textContent = "";

    if (filteredProducts.length === 0) {
        productGrid.innerHTML = `
            <div class="empty-state">
                <h3>No products found</h3>
                <p>Try another search or category.</p>
            </div>
        `;

        return;
    }

    productGrid.innerHTML = filteredProducts
        .map(product => {
            const price =
                Number(product.price_cents || 0) / 100;

            const imageUrl =
                product.image_url ||
                "https://via.placeholder.com/600x400?text=KaniMart";

            const stock =
                Number(product.stock_qty || 0);

            return `
                <article class="product-card">

                    <img
                        src="${escapeHtml(imageUrl)}"
                        alt="${escapeHtml(product.name || "Product")}"
                        onerror="this.src='https://via.placeholder.com/600x400?text=KaniMart'"
                    >

                    <div class="product-info">

                        <span class="category">
                            ${escapeHtml(product.category || "General")}
                        </span>

                        <h3>
                            ${escapeHtml(product.name || "Unnamed Product")}
                        </h3>

                        <p class="description">
                            ${escapeHtml(
                                product.description ||
                                "No description available."
                            )}
                        </p>

                        <div class="product-bottom">

                            <strong>
                                ₹${price.toLocaleString("en-IN", {
                                    minimumFractionDigits: 2
                                })}
                            </strong>

                            <span class="${stock > 0 ? "stock" : "out-stock"}">
                                ${
                                    stock > 0
                                        ? `${stock} in stock`
                                        : "Out of stock"
                                }
                            </span>

                        </div>

                        <button
                            class="primary-button add-cart"
                            data-product-id="${product.id}"
                            ${stock <= 0 ? "disabled" : ""}
                        >
                            Add to Cart
                        </button>

                        <div class="review-box">

                            <h4>Leave a Review</h4>

                            <select id="rating-${product.id}">
                                <option value="">Rating</option>
                                <option value="5">★★★★★ — 5</option>
                                <option value="4">★★★★☆ — 4</option>
                                <option value="3">★★★☆☆ — 3</option>
                                <option value="2">★★☆☆☆ — 2</option>
                                <option value="1">★☆☆☆☆ — 1</option>
                            </select>

                            <textarea
                                id="review-${product.id}"
                                placeholder="Write your review..."
                                rows="3"
                            ></textarea>

                            <button
                                class="secondary-button submit-review"
                                data-product-id="${product.id}"
                            >
                                Submit Review
                            </button>

                        </div>

                    </div>

                </article>
            `;
        })
        .join("");

    document.querySelectorAll(".add-cart").forEach(button => {
        button.addEventListener("click", () => {
            addToCart(
                Number(button.dataset.productId)
            );
        });
    });

    document.querySelectorAll(".submit-review").forEach(button => {
        button.addEventListener("click", () => {
            submitReview(
                Number(button.dataset.productId)
            );
        });
    });
    filteredProducts.forEach(product => {
    loadReviews(product.id);
    });
}

async function addToCart(productId) {
    const user = getUser();

    if (!user) {
        return;
    }

    const product = products.find(
        item => Number(item.id) === productId
    );

    if (!product) {
        return;
    }

    productMessage.textContent =
        `Adding ${product.name} to cart...`;

    try {
        const response = await fetch(
            `${API_BASE}/api/cart/${user.id}`,
            {
                method: "POST",
                headers: authHeaders(),
                body: JSON.stringify({
                    product_id: productId,
                    quantity: 1
                })
            }
        );

        const result = await response.json();

        if (!response.ok || !result.success) {
            throw new Error(
                result.error ||
                "Unable to add product to cart."
            );
        }

        await loadCart();

        productMessage.textContent =
            `${product.name} added to cart.`;

    } catch (error) {
        console.error(error);

        productMessage.textContent =
            error.message ||
            "Unable to add product to cart.";
    }
}


function escapeHtml(value) {
    return String(value)
        .replaceAll("&", "&amp;")
        .replaceAll("<", "&lt;")
        .replaceAll(">", "&gt;")
        .replaceAll('"', "&quot;")
        .replaceAll("'", "&#039;");
}


async function loadOrderHistory() {
    const user = getUser();

    if (!user) {
        return;
    }

    productMessage.textContent =
        "Loading order history...";

    try {
        const response = await fetch(
            `${API_BASE}/api/orders/${user.id}`,
            {
                method: "GET",
                headers: authHeaders()
            }
        );

        const result = await response.json();

        if (!response.ok || !result.success) {
            throw new Error(
                result.error ||
                "Unable to load order history."
            );
        }

        const orders =
            result.data?.orders || [];

        if (orders.length === 0) {
            productMessage.textContent =
                "No orders found.";

            return;
        }

        const summary = orders
            .map(order => {
                const amount =
                    Number(
                        order.total_amount_cents || 0
                    ) / 100;

                return (
                    `Order #${order.id} — ` +
                    `${order.status} — ` +
                    `₹${amount.toFixed(2)}`
                );
            })
            .join(" | ");

        productMessage.textContent =
            `Order History: ${summary}`;

    } catch (error) {
        console.error(error);

        productMessage.textContent =
            error.message ||
            "Unable to load order history.";
    }
}
async function submitReview(productId) {
    const user = getUser();

    if (!user) {
        return;
    }

    const ratingInput = document.getElementById(
        `rating-${productId}`
    );

    const commentInput = document.getElementById(
        `review-${productId}`
    );

    const rating = Number(ratingInput.value);
    const comment = commentInput.value.trim();

    if (rating < 1 || rating > 5) {
        productMessage.textContent =
            "Please select a rating from 1 to 5.";

        return;
    }

    if (!comment) {
        productMessage.textContent =
            "Please enter a review.";

        return;
    }

    productMessage.textContent =
        "Submitting review...";

    try {
        const response = await fetch(
            `${API_BASE}/api/products/${productId}/reviews/${user.id}`,
            {
                method: "POST",
                headers: authHeaders(),
                body: JSON.stringify({
                    rating,
                    comment
                })
            }
        );

        const result = await response.json();

        if (!response.ok || !result.success) {
            throw new Error(
                result.error ||
                result.message ||
                "Unable to submit review."
            );
        }

        productMessage.textContent =
            "Review submitted successfully.";

        ratingInput.value = "";
        commentInput.value = "";

    } catch (error) {
        console.error(error);

        productMessage.textContent =
            error.message ||
            "Unable to submit review.";
    }
}

loginForm.addEventListener(
    "submit",
    async event => {
        event.preventDefault();

        const email =
            document.getElementById("email").value.trim();

        const password =
            document.getElementById("password").value;

        message.textContent =
            "Logging in...";

        try {
            const response = await fetch(
                `${API_BASE}/api/auth/login`,
                {
                    method: "POST",
                    headers: {
                        "Content-Type": "application/json"
                    },
                    body: JSON.stringify({
                        email,
                        password
                    })
                }
            );

            const result = await response.json();

            if (!response.ok || !result.success) {
                message.textContent =
                    result.error ||
                    result.message ||
                    "Login failed.";

                return;
            }

            localStorage.setItem(
                "kanimart_token",
                result.data.token
            );

            localStorage.setItem(
                "kanimart_user",
                JSON.stringify(result.data.user)
            );

            message.textContent =
                `Welcome, ${result.data.user.name}!`;

            showMarketplace();

        } catch (error) {
            console.error(error);

            message.textContent =
                "Cannot connect to KaniMart server.";
        }
    }
);


logoutButton.addEventListener(
    "click",
    () => {
        localStorage.removeItem(
            "kanimart_token"
        );

        localStorage.removeItem(
            "kanimart_user"
        );

        showMarketplace();
    }
);


searchInput.addEventListener(
    "input",
    renderProducts
);


categoryFilter.addEventListener(
    "change",
    renderProducts
);


refreshProducts.addEventListener(
    "click",
    async () => {
        await loadProducts();
        await loadCart();
    }
);


cartButton.addEventListener(
    "click",
    async () => {
        await loadCart();

        const count = cart.items.reduce(
            (total, item) =>
                total + Number(item.quantity || 0),
            0
        );

        if (count === 0) {
            productMessage.textContent =
                "Your cart is empty.";

            return;
        }

        const summary = cart.items
            .map(item =>
                `${item.name} × ${item.quantity} = ₹${(
                    Number(item.subtotal_cents || 0) / 100
                ).toFixed(2)}`
            )
            .join(" | ");

        productMessage.textContent =
            `Cart: ${summary} | Total: ₹${(
                Number(cart.total_cents || 0) / 100
            ).toFixed(2)}`;
    }
);


orderHistoryButton.addEventListener(
    "click",
    loadOrderHistory
);


checkoutButton.addEventListener(
    "click",
    async () => {
        const user = getUser();

        if (!user) {
            return;
        }

        await loadCart();

        if (cart.items.length === 0) {
            productMessage.textContent =
                "Your cart is empty.";

            return;
        }

        productMessage.textContent =
            "Processing checkout...";

        try {
            const response = await fetch(
                `${API_BASE}/api/orders/checkout/${user.id}`,
                {
                    method: "POST",
                    headers: authHeaders()
                }
            );

            const result = await response.json();

            if (!response.ok || !result.success) {
                throw new Error(
                    result.error ||
                    result.message ||
                    "Checkout failed."
                );
            }

            cart = {
                items: [],
                total_cents: 0
            };

            updateCartCount();

            const ordersResponse = await fetch(
                `${API_BASE}/api/orders/${user.id}`,
                {
                    method: "GET",
                    headers: authHeaders()
                }
            );

            const ordersResult =
                await ordersResponse.json();

            if (
                ordersResponse.ok &&
                ordersResult.success &&
                ordersResult.data &&
                Array.isArray(
                    ordersResult.data.orders
                ) &&
                ordersResult.data.orders.length > 0
            ) {
                const latestOrder =
                    ordersResult.data.orders[0];

                productMessage.textContent =
                    `Checkout successful! Order #${latestOrder.id} created.`;
            } else {
                productMessage.textContent =
                    "Checkout successful! Your order was created.";
            }

        } catch (error) {
            console.error(error);

            productMessage.textContent =
                error.message ||
                "Unable to complete checkout.";
        }
    }
);

/* =========================================================
   KaniMart AI Assistant
   Backend: POST /api/chat
   ========================================================= */

function setupChatbot() {
    if (document.getElementById("kanimartChatbot")) {
        return;
    }

    const chatbot = document.createElement("section");

    chatbot.id = "kanimartChatbot";
    chatbot.className = "chatbot-section";

    chatbot.innerHTML = `
        <div class="chatbot-header">
            <div>
                <h2>KaniMart AI Assistant</h2>
                <p>Ask me about shopping and using KaniMart.</p>
            </div>
        </div>

        <div
            id="chatMessages"
            class="chat-messages"
        >
            <div class="chat-message assistant">
                <strong>KaniMart Assistant</strong>
                <span>
                    Hello! How can I help you with KaniMart?
                </span>
            </div>
        </div>

        <div class="chat-input-row">
            <input
                type="text"
                id="chatInput"
                placeholder="Ask about KaniMart..."
                maxlength="2000"
                autocomplete="off"
            />

            <button
                type="button"
                id="chatSendButton"
                class="primary-button"
            >
                Send
            </button>
        </div>

        <p
            id="chatStatus"
            class="chat-status"
            aria-live="polite"
        ></p>
    `;

    const marketplace =
        document.getElementById("marketplaceSection");

    if (marketplace) {
        marketplace.appendChild(chatbot);
    } else {
        document.body.appendChild(chatbot);
    }

    const chatInput =
        document.getElementById("chatInput");

    const chatSendButton =
        document.getElementById("chatSendButton");

    chatSendButton.addEventListener(
        "click",
        sendChatMessage
    );

    chatInput.addEventListener(
        "keydown",
        event => {
            if (event.key === "Enter") {
                event.preventDefault();
                sendChatMessage();
            }
        }
    );
}


async function sendChatMessage() {
    const input =
        document.getElementById("chatInput");

    const messages =
        document.getElementById("chatMessages");

    const status =
        document.getElementById("chatStatus");

    const button =
        document.getElementById("chatSendButton");

    if (!input || !messages || !status || !button) {
        return;
    }

    const userMessage =
        input.value.trim();

    if (!userMessage) {
        return;
    }

    /*
     * Display the user's message safely.
     * textContent is used instead of innerHTML
     * to prevent HTML/script injection.
     */
    const userMessageElement =
        document.createElement("div");

    userMessageElement.className =
        "chat-message user";

    const userLabel =
        document.createElement("strong");

    userLabel.textContent = "You";

    const userText =
        document.createElement("span");

    userText.textContent = userMessage;

    userMessageElement.appendChild(userLabel);
    userMessageElement.appendChild(userText);

    messages.appendChild(userMessageElement);

    input.value = "";
    button.disabled = true;

    status.textContent =
        "KaniMart Assistant is thinking...";

    messages.scrollTop =
        messages.scrollHeight;

    try {
        const response = await fetch(
            `${API_BASE}/api/chat`,
            {
                method: "POST",
                headers: {
                    "Content-Type": "application/json"
                },
                body: JSON.stringify({
                    message: userMessage
                })
            }
        );

        const result =
            await response.json();

        if (!response.ok || !result.success) {
            throw new Error(
                result.error ||
                "Unable to contact the AI assistant."
            );
        }

        const answer =
            result.data?.message;

        if (!answer) {
            throw new Error(
                "AI assistant returned an empty response."
            );
        }

        const assistantMessageElement =
            document.createElement("div");

        assistantMessageElement.className =
            "chat-message assistant";

        const assistantLabel =
            document.createElement("strong");

        assistantLabel.textContent =
            "KaniMart Assistant";

        const assistantText =
            document.createElement("span");

        assistantText.textContent =
            answer;

        assistantMessageElement.appendChild(
            assistantLabel
        );

        assistantMessageElement.appendChild(
            assistantText
        );

        messages.appendChild(
            assistantMessageElement
        );

        status.textContent = "";

    } catch (error) {
        console.error(
            "KaniMart AI error:",
            error
        );

        status.textContent =
            error.message ||
            "Unable to contact the AI assistant.";
    } finally {
        button.disabled = false;
        input.focus();

        messages.scrollTop =
            messages.scrollHeight;
    }
}


setupChatbot();

updateCartCount();
showMarketplace();
