const API_BASE = "http://127.0.0.1:8080";

const authSection =
    document.getElementById("authSection");

const marketplaceSection =
    document.getElementById("marketplaceSection");

const loginForm =
    document.getElementById("loginForm");

const registerForm =
    document.getElementById("registerForm");

const message =
    document.getElementById("message");

const welcomeUser =
    document.getElementById("welcomeUser");

const logoutButton =
    document.getElementById("logoutButton");

const searchInput =
    document.getElementById("searchInput");

const categoryFilter =
    document.getElementById("categoryFilter");

const refreshProducts =
    document.getElementById("refreshProducts");

const productGrid =
    document.getElementById("productGrid");

const productCount =
    document.getElementById("productCount");

const productMessage =
    document.getElementById("productMessage");

const cartButton =
    document.getElementById("cartButton");

const cartCount =
    document.getElementById("cartCount");

const checkoutButton =
    document.getElementById("checkoutButton");

const orderHistoryButton =
    document.getElementById("orderHistoryButton");

const sellerManagement =
    document.getElementById("sellerManagement");


let products = [];

let cart = {
    items: [],
    total_cents: 0
};


/* =========================================================
   AUTH
   ========================================================= */

function getToken() {

    return localStorage.getItem(
        "kanimart_token"
    );
}


function getUser() {

    const value =
        localStorage.getItem(
            "kanimart_user"
        );

    if (!value) {
        return null;
    }

    try {

        return JSON.parse(value);

    } catch (error) {

        console.error(
            "Invalid saved user:",
            error
        );

        return null;
    }
}


function authHeaders() {

    const token =
        getToken();

    return {
        "Content-Type":
            "application/json",

        "Authorization":
            `Bearer ${token}`
    };
}


/* =========================================================
   SHOW LOGIN
   ========================================================= */

function showLogin() {

    if (authSection) {

        authSection.classList.remove(
            "hidden"
        );
    }


    if (marketplaceSection) {

        marketplaceSection.classList.add(
            "hidden"
        );
    }


    if (welcomeUser) {

        welcomeUser.textContent = "";
    }


    if (sellerManagement) {

        sellerManagement.hidden = true;
    }
}


/* =========================================================
   SHOW MARKETPLACE
   ========================================================= */

function showMarketplace() {

    const user =
        getUser();

    const token =
        getToken();


    /*
     * No valid saved session.
     */

    if (!user || !token) {

        showLogin();

        return;
    }


    /*
     * Hide authentication.
     */

    if (authSection) {

        authSection.classList.add(
            "hidden"
        );
    }


    /*
     * Show marketplace.
     */

    if (marketplaceSection) {

        marketplaceSection.classList.remove(
            "hidden"
        );
    }


    /*
     * Welcome message.
     */

    if (welcomeUser) {

        welcomeUser.textContent =
            `${user.name} (${user.role})`;
    }


    /*
     * Seller Management:
     *
     * SELLER -> visible
     * ADMIN  -> visible
     * BUYER  -> hidden
     */

    if (sellerManagement) {

        if (
            user.role === "SELLER" ||
            user.role === "ADMIN"
        ) {

            sellerManagement.hidden = false;

        } else {

            sellerManagement.hidden = true;
        }
    }


    updateCartCount();

    loadProducts();

    loadCart();
}


/* =========================================================
   REGISTRATION
   ========================================================= */

if (registerForm) {

    registerForm.addEventListener(
        "submit",
        async event => {

            event.preventDefault();


            const registerName =
                document.getElementById(
                    "registerName"
                );

            const registerEmail =
                document.getElementById(
                    "registerEmail"
                );

            const registerPassword =
                document.getElementById(
                    "registerPassword"
                );

            const registerRole =
                document.getElementById(
                    "registerRole"
                );


            /*
             * Safety check.
             */

            if (
                !registerName ||
                !registerEmail ||
                !registerPassword ||
                !registerRole
            ) {

                console.error(
                    "Registration form elements are missing."
                );

                return;
            }


            const name =
                registerName.value.trim();

            const email =
                registerEmail.value.trim();

            const password =
                registerPassword.value;

            const role =
                registerRole.value;


            /*
             * Client-side validation.
             */

            if (!name) {

                message.textContent =
                    "Please enter your name.";

                return;
            }


            if (!email) {

                message.textContent =
                    "Please enter your email.";

                return;
            }


            if (password.length < 8) {

                message.textContent =
                    "Password must be at least 8 characters.";

                return;
            }


            /*
             * Only Buyer and Seller are
             * allowed through public registration.
             */

            if (
                role !== "BUYER" &&
                role !== "SELLER"
            ) {

                message.textContent =
                    "Please select Buyer or Seller.";

                return;
            }


            message.textContent =
                "Creating account...";


            try {

                const response =
                    await fetch(
                        `${API_BASE}/api/auth/register`,
                        {
                            method: "POST",

                            headers: {
                                "Content-Type":
                                    "application/json"
                            },

                            body: JSON.stringify({
                                name,
                                email,
                                password,
                                role
                            })
                        }
                    );


                const result =
                    await response.json();


                /*
                 * Backend returned an error.
                 */

                if (
                    !response.ok ||
                    !result.success
                ) {

                    throw new Error(
                        result.error ||
                        result.message ||
                        "Registration failed."
                    );
                }


                /*
                 * Successful registration.
                 */

                message.textContent =
                    `Account created successfully as ${role}. You can now login.`;


                /*
                 * Clear registration form.
                 */

                registerForm.reset();


                /*
                 * Put the email into login form
                 * to make login easier.
                 */

                const loginEmail =
                    document.getElementById(
                        "email"
                    );

                if (loginEmail) {

                    loginEmail.value =
                        email;
                }


                /*
                 * Put focus on password field.
                 */

                const loginPassword =
                    document.getElementById(
                        "password"
                    );

                if (loginPassword) {

                    loginPassword.focus();
                }

            } catch (error) {

                console.error(
                    "Registration error:",
                    error
                );


                message.textContent =
                    error.message ||
                    "Unable to create account.";
            }
        }
    );
}


/* =========================================================
   CART COUNT
   ========================================================= */

function updateCartCount() {

    if (!cartCount) {
        return;
    }


    const items =
        Array.isArray(cart.items)
            ? cart.items
            : [];


    const count =
        items.reduce(
            (total, item) =>
                total +
                Number(
                    item.quantity || 0
                ),
            0
        );


    cartCount.textContent =
        count;
}


/* =========================================================
   PRODUCTS
   ========================================================= */

async function loadProducts() {

    if (!productGrid) {
        return;
    }


    if (productMessage) {

        productMessage.textContent =
            "Loading products...";
    }


    productGrid.innerHTML = "";


    try {

        const response =
            await fetch(
                `${API_BASE}/api/products`
            );


        const result =
            await response.json();


        if (!response.ok) {

            throw new Error(
                result.error ||
                result.message ||
                "Unable to load products."
            );
        }


        products =
            Array.isArray(result)
                ? result
                : Array.isArray(result.data)
                    ? result.data
                    : [];


        populateCategories();

        renderProducts();

    } catch (error) {

        console.error(
            "Product loading error:",
            error
        );


        if (productCount) {

            productCount.textContent =
                "0 products";
        }


        if (productMessage) {

            productMessage.textContent =
                "Unable to load products from KaniMart.";
        }
    }
}


/* =========================================================
   PRODUCT CATEGORIES
   ========================================================= */

function populateCategories() {

    if (!categoryFilter) {
        return;
    }


    const currentCategory =
        categoryFilter.value;


    const categories = [
        ...new Set(
            products
                .map(
                    product =>
                        product.category
                )
                .filter(Boolean)
        )
    ].sort();


    categoryFilter.innerHTML =
        '<option value="">All categories</option>';


    categories.forEach(
        category => {

            const option =
                document.createElement(
                    "option"
                );


            option.value =
                category;

            option.textContent =
                category;


            categoryFilter.appendChild(
                option
            );
        }
    );


    categoryFilter.value =
        currentCategory;
}


/* =========================================================
   FILTER PRODUCTS
   ========================================================= */

function getFilteredProducts() {

    const search =
        searchInput
            ? searchInput.value
                .trim()
                .toLowerCase()
            : "";


    const category =
        categoryFilter
            ? categoryFilter.value
            : "";


    return products.filter(
        product => {

            const matchesSearch =
                !search ||
                String(
                    product.name || ""
                )
                    .toLowerCase()
                    .includes(search) ||

                String(
                    product.description || ""
                )
                    .toLowerCase()
                    .includes(search);


            const matchesCategory =
                !category ||
                product.category === category;


            return (
                matchesSearch &&
                matchesCategory
            );
        }
    );
}


/* =========================================================
   RENDER PRODUCTS
   ========================================================= */

function renderProducts() {

    if (!productGrid) {
        return;
    }


    const filteredProducts =
        getFilteredProducts();


    if (productCount) {

        productCount.textContent =
            `${filteredProducts.length} product${
                filteredProducts.length === 1
                    ? ""
                    : "s"
            }`;
    }


    if (productMessage) {

        productMessage.textContent = "";
    }


    if (filteredProducts.length === 0) {

        productGrid.innerHTML = `
            <div class="empty-state">
                <h3>No products found</h3>
                <p>
                    Try another search or category.
                </p>
            </div>
        `;

        return;
    }


    productGrid.innerHTML =
        filteredProducts
            .map(product => {

                const price =
                    Number(
                        product.price_cents || 0
                    ) / 100;


                const imageUrl =
                    product.image_url ||
                    "https://via.placeholder.com/600x400?text=KaniMart";


                const stock =
                    Number(
                        product.stock_qty || 0
                    );


                return `
                    <article class="product-card">

                        <img
                            src="${escapeHtml(imageUrl)}"
                            alt="${escapeHtml(
                                product.name ||
                                "Product"
                            )}"
                            onerror="
                                this.src='https://via.placeholder.com/600x400?text=KaniMart'
                            "
                        >

                        <div class="product-info">

                            <span class="category">
                                ${escapeHtml(
                                    product.category ||
                                    "General"
                                )}
                            </span>


                            <h3>
                                ${escapeHtml(
                                    product.name ||
                                    "Unnamed Product"
                                )}
                            </h3>


                            <p class="description">
                                ${escapeHtml(
                                    product.description ||
                                    "No description available."
                                )}
                            </p>


                            <div class="product-bottom">

                                <strong>
                                    ₹${price.toLocaleString(
                                        "en-IN",
                                        {
                                            minimumFractionDigits:
                                                2
                                        }
                                    )}
                                </strong>


                                <span class="${
                                    stock > 0
                                        ? "stock"
                                        : "out-stock"
                                }">

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
                                ${stock <= 0
                                    ? "disabled"
                                    : ""}
                            >
                                Add to Cart
                            </button>


                            <!-- =====================
                                 REVIEW
                                 ===================== -->

                            <div class="review-box">

                                <h4>
                                    Leave a Review
                                </h4>


                                <select
                                    id="rating-${product.id}"
                                >

                                    <option value="">
                                        Rating
                                    </option>

                                    <option value="5">
                                        ★★★★★ — 5
                                    </option>

                                    <option value="4">
                                        ★★★★☆ — 4
                                    </option>

                                    <option value="3">
                                        ★★★☆☆ — 3
                                    </option>

                                    <option value="2">
                                        ★★☆☆☆ — 2
                                    </option>

                                    <option value="1">
                                        ★☆☆☆☆ — 1
                                    </option>

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


                            <div
                                id="reviews-${product.id}"
                                class="reviews"
                            >
                                Loading reviews...
                            </div>

                        </div>

                    </article>
                `;
            })
            .join("");


    /*
     * Add-to-cart listeners.
     */

    document
        .querySelectorAll(".add-cart")
        .forEach(
            button => {

                button.addEventListener(
                    "click",
                    () => {

                        addToCart(
                            Number(
                                button.dataset.productId
                            )
                        );
                    }
                );
            }
        );


    /*
     * Review listeners.
     */

    document
        .querySelectorAll(".submit-review")
        .forEach(
            button => {

                button.addEventListener(
                    "click",
                    () => {

                        submitReview(
                            Number(
                                button.dataset.productId
                            )
                        );
                    }
                );
            }
        );


    /*
     * Load reviews for every visible product.
     */

    filteredProducts.forEach(
        product => {

            loadReviews(
                product.id
            );
        }
    );
}


/* =========================================================
   CART
   ========================================================= */

async function loadCart() {

    const user =
        getUser();


    if (!user) {
        return;
    }


    try {

        const response =
            await fetch(
                `${API_BASE}/api/cart/${user.id}`,
                {
                    method: "GET",
                    headers: authHeaders()
                }
            );


        const result =
            await response.json();


        if (
            !response.ok ||
            !result.success
        ) {

            throw new Error(
                result.error ||
                "Unable to load cart."
            );
        }


        if (
            Array.isArray(
                result.data
            )
        ) {

            cart = {
                items:
                    result.data,

                total_cents:
                    0
            };

        } else {

            cart =
                result.data || {
                    items: [],
                    total_cents: 0
                };
        }


        if (
            !Array.isArray(
                cart.items
            )
        ) {

            cart.items = [];
        }


        updateCartCount();

    } catch (error) {

        console.error(
            "Cart loading error:",
            error
        );


        cart = {
            items: [],
            total_cents: 0
        };


        updateCartCount();
    }
}


/* =========================================================
   ADD TO CART
   ========================================================= */

async function addToCart(productId) {

    const user =
        getUser();


    if (!user) {

        showLogin();

        return;
    }


    const product =
        products.find(
            item =>
                Number(item.id) ===
                productId
        );


    if (!product) {
        return;
    }


    if (productMessage) {

        productMessage.textContent =
            `Adding ${product.name} to cart...`;
    }


    try {

        const response =
            await fetch(
                `${API_BASE}/api/cart/${user.id}`,
                {
                    method: "POST",

                    headers:
                        authHeaders(),

                    body:
                        JSON.stringify({
                            product_id:
                                productId,

                            quantity:
                                1
                        })
                }
            );


        const result =
            await response.json();


        if (
            !response.ok ||
            !result.success
        ) {

            throw new Error(
                result.error ||
                result.message ||
                "Unable to add product to cart."
            );
        }


        await loadCart();


        if (productMessage) {

            productMessage.textContent =
                `${product.name} added to cart.`;
        }

    } catch (error) {

        console.error(
            "Add to cart error:",
            error
        );


        if (productMessage) {

            productMessage.textContent =
                error.message ||
                "Unable to add product to cart.";
        }
    }
}


/* =========================================================
   REVIEWS
   ========================================================= */

async function loadReviews(productId) {

    const reviewContainer =
        document.getElementById(
            `reviews-${productId}`
        );


    if (!reviewContainer) {
        return;
    }


    try {

        const response =
            await fetch(
                `${API_BASE}/api/products/${productId}/reviews`
            );


        const result =
            await response.json();


        if (
            !response.ok ||
            !result.success
        ) {

            reviewContainer.innerHTML =
                "<p>No reviews available.</p>";

            return;
        }


        const reviews =
            result.data?.reviews || [];


        if (reviews.length === 0) {

            reviewContainer.innerHTML =
                "<p>No reviews yet.</p>";

            return;
        }


        reviewContainer.innerHTML =
            reviews
                .map(
                    review => {

                        const rating =
                            Number(
                                review.rating ||
                                0
                            );


                        const stars =
                            "★".repeat(
                                rating
                            ) +
                            "☆".repeat(
                                Math.max(
                                    0,
                                    5 - rating
                                )
                            );


                        return `
                            <div class="review-item">

                                <strong>
                                    ${stars}
                                </strong>

                                <p>
                                    ${escapeHtml(
                                        review.comment ||
                                        ""
                                    )}
                                </p>

                            </div>
                        `;
                    }
                )
                .join("");

    } catch (error) {

        console.error(
            "Review loading error:",
            error
        );


        reviewContainer.innerHTML =
            "<p>Unable to load reviews.</p>";
    }
}


/* =========================================================
   SUBMIT REVIEW
   ========================================================= */

async function submitReview(productId) {

    const user =
        getUser();


    if (!user) {

        showLogin();

        return;
    }


    const ratingInput =
        document.getElementById(
            `rating-${productId}`
        );


    const commentInput =
        document.getElementById(
            `review-${productId}`
        );


    if (
        !ratingInput ||
        !commentInput
    ) {

        return;
    }


    const rating =
        Number(
            ratingInput.value
        );


    const comment =
        commentInput.value.trim();


    if (
        rating < 1 ||
        rating > 5
    ) {

        if (productMessage) {

            productMessage.textContent =
                "Please select a rating from 1 to 5.";
        }

        return;
    }


    if (!comment) {

        if (productMessage) {

            productMessage.textContent =
                "Please enter a review.";
        }

        return;
    }


    if (productMessage) {

        productMessage.textContent =
            "Submitting review...";
    }


    try {

        const response =
            await fetch(
                `${API_BASE}/api/products/${productId}/reviews/${user.id}`,
                {
                    method: "POST",

                    headers:
                        authHeaders(),

                    body:
                        JSON.stringify({
                            rating,
                            comment
                        })
                }
            );


        const result =
            await response.json();


        if (
            !response.ok ||
            !result.success
        ) {

            throw new Error(
                result.error ||
                result.message ||
                "Unable to submit review."
            );
        }


        if (productMessage) {

            productMessage.textContent =
                "Review submitted successfully.";
        }


        ratingInput.value = "";

        commentInput.value = "";


        await loadReviews(
            productId
        );

    } catch (error) {

        console.error(
            "Review submission error:",
            error
        );


        if (productMessage) {

            productMessage.textContent =
                error.message ||
                "Unable to submit review.";
        }
    }
}


/* =========================================================
   LOGIN
   ========================================================= */

if (loginForm) {

    loginForm.addEventListener(
        "submit",
        async event => {

            event.preventDefault();


            const email =
                document
                    .getElementById(
                        "email"
                    )
                    .value
                    .trim();


            const password =
                document
                    .getElementById(
                        "password"
                    )
                    .value;


            if (message) {

                message.textContent =
                    "Logging in...";
            }


            try {

                const response =
                    await fetch(
                        `${API_BASE}/api/auth/login`,
                        {
                            method: "POST",

                            headers: {
                                "Content-Type":
                                    "application/json"
                            },

                            body:
                                JSON.stringify({
                                    email,
                                    password
                                })
                        }
                    );


                const result =
                    await response.json();


                if (
                    !response.ok ||
                    !result.success
                ) {

                    if (message) {

                        message.textContent =
                            result.error ||
                            result.message ||
                            "Login failed.";
                    }

                    return;
                }


                /*
                 * Save JWT.
                 */

                localStorage.setItem(
                    "kanimart_token",
                    result.data.token
                );


                /*
                 * Save user.
                 */

                localStorage.setItem(
                    "kanimart_user",
                    JSON.stringify(
                        result.data.user
                    )
                );


                if (message) {

                    message.textContent =
                        `Welcome, ${result.data.user.name}!`;
                }


                /*
                 * Show marketplace.
                 */

                showMarketplace();

            } catch (error) {

                console.error(
                    "Login error:",
                    error
                );


                if (message) {

                    message.textContent =
                        "Cannot connect to KaniMart server.";
                }
            }
        }
    );
}


/* =========================================================
   LOGOUT
   ========================================================= */

if (logoutButton) {

    logoutButton.addEventListener(
        "click",
        () => {

            localStorage.removeItem(
                "kanimart_token"
            );


            localStorage.removeItem(
                "kanimart_user"
            );


            cart = {
                items: [],
                total_cents: 0
            };


            showLogin();
        }
    );
}


/* =========================================================
   SEARCH
   ========================================================= */

if (searchInput) {

    searchInput.addEventListener(
        "input",
        renderProducts
    );
}


/* =========================================================
   CATEGORY FILTER
   ========================================================= */

if (categoryFilter) {

    categoryFilter.addEventListener(
        "change",
        renderProducts
    );
}


/* =========================================================
   REFRESH PRODUCTS
   ========================================================= */

if (refreshProducts) {

    refreshProducts.addEventListener(
        "click",
        async () => {

            await loadProducts();

            await loadCart();
        }
    );
}


/* =========================================================
   CART NAVIGATION
   ========================================================= */

if (cartButton) {

    cartButton.addEventListener(
        "click",
        () => {

            window.location.href =
                "cart.html";
        }
    );
}


/* =========================================================
   ORDER HISTORY
   ========================================================= */

if (orderHistoryButton) {

    orderHistoryButton.addEventListener(
        "click",
        () => {

            window.location.href =
                "orders.html";
        }
    );
}


/* =========================================================
   CHECKOUT
   ========================================================= */

if (checkoutButton) {

    checkoutButton.addEventListener(
        "click",
        () => {

            window.location.href =
                "checkout.html";
        }
    );
}


/* =========================================================
   HTML ESCAPING
   ========================================================= */

function escapeHtml(value) {

    return String(value)
        .replaceAll(
            "&",
            "&amp;"
        )
        .replaceAll(
            "<",
            "&lt;"
        )
        .replaceAll(
            ">",
            "&gt;"
        )
        .replaceAll(
            '"',
            "&quot;"
        )
        .replaceAll(
            "'",
            "&#039;"
        );
}


/* =========================================================
   AI CHATBOT
   ========================================================= */

function setupChatbot() {

    if (
        document.getElementById(
            "kanimartChatbot"
        )
    ) {

        return;
    }


    const chatbot =
        document.createElement(
            "section"
        );


    chatbot.id =
        "kanimartChatbot";


    chatbot.className =
        "chatbot-section";


    chatbot.innerHTML = `

        <div class="chatbot-header">

            <div>

                <h2>
                    KaniMart AI Assistant
                </h2>

                <p>
                    Ask me about shopping and using KaniMart.
                </p>

            </div>

        </div>


        <div
            id="chatMessages"
            class="chat-messages"
        >

            <div class="chat-message assistant">

                <strong>
                    KaniMart Assistant
                </strong>

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
        document.getElementById(
            "marketplaceSection"
        );


    if (marketplace) {

        marketplace.appendChild(
            chatbot
        );

    } else {

        document.body.appendChild(
            chatbot
        );
    }


    const chatInput =
        document.getElementById(
            "chatInput"
        );


    const chatSendButton =
        document.getElementById(
            "chatSendButton"
        );


    if (chatSendButton) {

        chatSendButton.addEventListener(
            "click",
            sendChatMessage
        );
    }


    if (chatInput) {

        chatInput.addEventListener(
            "keydown",
            event => {

                if (
                    event.key ===
                    "Enter"
                ) {

                    event.preventDefault();

                    sendChatMessage();
                }
            }
        );
    }
}


/* =========================================================
   SEND CHAT MESSAGE
   ========================================================= */

async function sendChatMessage() {

    const input =
        document.getElementById(
            "chatInput"
        );


    const messages =
        document.getElementById(
            "chatMessages"
        );


    const status =
        document.getElementById(
            "chatStatus"
        );


    const button =
        document.getElementById(
            "chatSendButton"
        );


    if (
        !input ||
        !messages ||
        !status ||
        !button
    ) {

        return;
    }


    const userMessage =
        input.value.trim();


    if (!userMessage) {
        return;
    }


    /*
     * User message.
     */

    const userMessageElement =
        document.createElement(
            "div"
        );


    userMessageElement.className =
        "chat-message user";


    const userLabel =
        document.createElement(
            "strong"
        );


    userLabel.textContent =
        "You";


    const userText =
        document.createElement(
            "span"
        );


    userText.textContent =
        userMessage;


    userMessageElement.appendChild(
        userLabel
    );


    userMessageElement.appendChild(
        userText
    );


    messages.appendChild(
        userMessageElement
    );


    input.value = "";

    button.disabled = true;


    status.textContent =
        "KaniMart Assistant is thinking...";


    messages.scrollTop =
        messages.scrollHeight;


    try {

        const response =
            await fetch(
                `${API_BASE}/api/chat`,
                {
                    method: "POST",

                    headers: {
                        "Content-Type":
                            "application/json"
                    },

                    body:
                        JSON.stringify({
                            message:
                                userMessage
                        })
                }
            );


        const result =
            await response.json();


        if (
            !response.ok ||
            !result.success
        ) {

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


        /*
         * Assistant response.
         */

        const assistantMessageElement =
            document.createElement(
                "div"
            );


        assistantMessageElement.className =
            "chat-message assistant";


        const assistantLabel =
            document.createElement(
                "strong"
            );


        assistantLabel.textContent =
            "KaniMart Assistant";


        const assistantText =
            document.createElement(
                "span"
            );


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


/* =========================================================
   INITIALIZATION
   ========================================================= */

document.addEventListener(
    "DOMContentLoaded",
    () => {

        /*
         * Create chatbot.
         */

        setupChatbot();


        /*
         * Restore saved login session.
         */

        const token =
            localStorage.getItem(
                "kanimart_token"
            );


        const savedUser =
            localStorage.getItem(
                "kanimart_user"
            );


        if (
            token &&
            savedUser
        ) {

            try {

                const user =
                    JSON.parse(
                        savedUser
                    );


                if (
                    user &&
                    user.id
                ) {

                    showMarketplace();

                    return;
                }

            } catch (error) {

                console.error(
                    "Unable to restore session:",
                    error
                );
            }
        }


        /*
         * No valid session.
         */

        showLogin();
    }
);