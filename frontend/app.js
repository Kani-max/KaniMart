const API_BASE = window.KANIMART_API_BASE ||
    (window.location.protocol === "http:" &&
        (window.location.hostname === "127.0.0.1" ||
            window.location.hostname === "localhost")
        ? "http://127.0.0.1:8080"
        : "");

const $ = (id) => document.getElementById(id);

const authSection = $("authSection");
const marketplaceSection =
    $("marketplace") || $("marketplaceSection");
// ===============================
// AUTH ELEMENTS
// ===============================

const loginView = $("loginView");
const registerView = $("registerView");

const loginForm = $("loginForm");
const registerForm = $("registerForm");

const showRegisterButton = $("showRegisterButton");
const showLoginButton = $("showLoginButton");

const loginMessage = $("loginMessage");
const registerMessage = $("registerMessage");
    
const message = $("message");
const welcomeUser = $("welcomeUser");
const logoutButton = $("logoutButton");

const searchInput = $("searchInput");
const categoryFilter = $("categoryFilter");
const refreshProducts = $("refreshProducts");

const productGrid = $("productGrid");
const productCount = $("productCount");
const productMessage = $("productMessage");

const cartButton = $("cartButton");
const cartCount = $("cartCount");
const checkoutButton = $("checkoutButton");
const orderHistoryButton = $("orderHistoryButton");

const sellerManagement = $("sellerManagement");

let products = [];

let cart = {
    items: [],
    total_cents: 0
};


/* =========================================================
   AUTHENTICATION HELPERS
   ========================================================= */

function getToken() {
    return localStorage.getItem("kanimart_token");
}


function getUser() {
    try {
        return JSON.parse(
            localStorage.getItem("kanimart_user") || "null"
        );
    } catch (error) {
        console.error("Invalid saved user:", error);
        return null;
    }
}


function authHeaders() {
    const headers = {
        "Content-Type": "application/json"
    };

    const token = getToken();

    if (token) {
        headers.Authorization = `Bearer ${token}`;
    }

    return headers;
}


/* =========================================================
   HTML ESCAPE
   ========================================================= */

function escapeHtml(value) {
    return String(value ?? "")
        .replaceAll("&", "&amp;")
        .replaceAll("<", "&lt;")
        .replaceAll(">", "&gt;")
        .replaceAll('"', "&quot;")
        .replaceAll("'", "&#039;");
}


/* =========================================================
   SHOW LOGIN
   ========================================================= */

function showLogin() {

    document.body.classList.add("auth-mode");

    if (authSection) {
        authSection.style.display = "";
        authSection.classList.remove("hidden");
    }

    if (marketplaceSection) {
        marketplaceSection.style.display = "none";
        marketplaceSection.classList.add("hidden");
    }

    if (welcomeUser) {
        welcomeUser.textContent = "";
    }

    if (sellerManagement) {
        sellerManagement.hidden = true;
        sellerManagement.style.display = "none";
    }
}


/* =========================================================
   SHOW MARKETPLACE
   ========================================================= */

async function showMarketplace() {

    const user = getUser();

    if (!user || !getToken()) {
        showLogin();
        return;
    }

    document.body.classList.remove("auth-mode");

    if (authSection) {
        authSection.style.display = "none";
        authSection.classList.add("hidden");
    }

    if (marketplaceSection) {
        marketplaceSection.style.display = "";
        marketplaceSection.classList.remove("hidden");
    }

    if (welcomeUser) {
        welcomeUser.textContent =
            `${user.name} (${user.role})`;
    }

    if (sellerManagement) {

        const allowed =
            user.role === "SELLER" ||
            user.role === "ADMIN";

        sellerManagement.hidden = !allowed;

        sellerManagement.style.display =
            allowed ? "inline-flex" : "none";
    }

    await loadProducts();
    await loadCart();
}
/* =========================================================
   LOGIN
   ========================================================= */

if (loginForm) {

    loginForm.addEventListener("submit", async (event) => {

        event.preventDefault();

        const emailInput = $("email");
        const passwordInput = $("password");

        if (!emailInput || !passwordInput) {
            return;
        }

        const email = emailInput.value.trim();
        const password = passwordInput.value;

        if (!email || !password) {

            if (message) {
                message.textContent =
                    "Please enter your email and password.";
            }

            return;
        }

        if (message) {
            message.textContent = "Logging in...";
        }

        try {

            const response = await fetch(
                `${API_BASE}/api/auth/login`,
                {
                    method: "POST",

                    headers: {
                        "Content-Type":
                            "application/json"
                    },

                    body: JSON.stringify({
                        email,
                        password
                    })
                }
            );

            const result = await response.json();

            if (!response.ok ||
                result.success === false) {

                throw new Error(
                    result.error ||
                    result.message ||
                    "Login failed."
                );
            }

            /*
             * Backend response:
             *
             * result.data.token
             * result.data.user
             */

            const data = result.data;

            if (!data ||
                !data.token ||
                !data.user) {

                throw new Error(
                    "Login response is missing user or token."
                );
            }

            localStorage.setItem(
                "kanimart_token",
                data.token
            );

            localStorage.setItem(
                "kanimart_user",
                JSON.stringify(data.user)
            );

            if (message) {
                message.textContent =
                    `Welcome, ${data.user.name}!`;
            }

            await showMarketplace();

        } catch (error) {

            console.error(
                "Login error:",
                error
            );

            if (message) {
                message.textContent =
                    error.message ||
                    "Unable to login.";
            }
        }
    });
}


/* =========================================================
   REGISTRATION
   ========================================================= */

if (registerForm) {

    registerForm.addEventListener(
        "submit",
        async (event) => {

            event.preventDefault();

            const nameInput =
                $("registerName");

            const emailInput =
                $("registerEmail");

            const passwordInput =
                $("registerPassword");

            const roleInput =
                $("registerRole");

            if (!nameInput ||
                !emailInput ||
                !passwordInput ||
                !roleInput) {

                console.error(
                    "Registration form elements are missing."
                );

                return;
            }

            const name =
                nameInput.value.trim();

            const email =
                emailInput.value.trim();

            const password =
                passwordInput.value;

            const role =
                roleInput.value;


            /* ---------- Validation ---------- */

            if (!name) {

                if (message) {
                    message.textContent =
                        "Please enter your name.";
                }

                return;
            }


            if (!email) {

                if (message) {
                    message.textContent =
                        "Please enter your email.";
                }

                return;
            }


            if (password.length < 8) {

                if (message) {
                    message.textContent =
                        "Password must be at least 8 characters.";
                }

                return;
            }


            /*
             * Public registration supports
             * BUYER and SELLER.
             *
             * ADMIN accounts should be
             * created separately.
             */

            if (role !== "BUYER" &&
                role !== "SELLER") {

                if (message) {
                    message.textContent =
                        "Please select Buyer or Seller.";
                }

                return;
            }


            if (message) {
                message.textContent =
                    "Creating your account...";
            }


            try {

                const response = await fetch(
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


                if (!response.ok ||
                    result.success === false) {

                    throw new Error(
                        result.error ||
                        result.message ||
                        "Registration failed."
                    );
                }


                if (message) {

                    message.textContent =
                        `Account created successfully as ${role}. Please login.`;
                }


                /*
                 * Clear registration form.
                 */

                registerForm.reset();


                /*
                 * Put the newly registered
                 * email into the login form.
                 */

                if ($("email")) {
                    $("email").value = email;
                }


                /*
                 * Focus password field.
                 */

                if ($("password")) {
                    $("password").focus();
                }

            } catch (error) {

                console.error(
                    "Registration error:",
                    error
                );

                if (message) {
                    message.textContent =
                        error.message ||
                        "Unable to create account.";
                }
            }
        }
    );
}
/* =========================================================
   LOGOUT
   ========================================================= */

if (logoutButton) {

    logoutButton.addEventListener("click", () => {

        localStorage.removeItem("kanimart_token");
        localStorage.removeItem("kanimart_user");

        cart = {
            items: [],
            total_cents: 0
        };

        products = [];

        if (productGrid) {
            productGrid.innerHTML = "";
        }

        if (cartCount) {
            cartCount.textContent = "0";
        }

        if (message) {
            message.textContent = "";
        }

        showLogin();
    });
}


/* =========================================================
   LOAD PRODUCTS
   ========================================================= */

async function loadProducts() {

    if (!productGrid) {
        return;
    }

    productGrid.innerHTML = `
        <div class="loading">
            Loading products...
        </div>
    `;

    if (productMessage) {
        productMessage.textContent = "";
    }

    try {

        const response = await fetch(
            `${API_BASE}/api/products`,
            {
                method: "GET",
                headers: authHeaders()
            }
        );

        const result = await response.json();

        if (!response.ok ||
            result.success === false) {

            throw new Error(
                result.error ||
                result.message ||
                "Unable to load products."
            );
        }

        /*
         * Expected backend response:
         *
         * {
         *   success: true,
         *   data: [...]
         * }
         */

        products =
            Array.isArray(result.data)
                ? result.data
                : [];

        populateCategories();
        renderProducts();

    } catch (error) {

        console.error(
            "Product loading error:",
            error
        );

        products = [];

        productGrid.innerHTML = `
            <div class="empty-state">
                <h3>Unable to load products</h3>
                <p>${escapeHtml(error.message)}</p>
            </div>
        `;

        if (productCount) {
            productCount.textContent = "0 products";
        }

        if (productMessage) {
            productMessage.textContent =
                error.message ||
                "Unable to load products.";
        }
    }
}


/* =========================================================
   POPULATE CATEGORY FILTER
   ========================================================= */

function populateCategories() {

    if (!categoryFilter) {
        return;
    }

    const currentValue =
        categoryFilter.value;

    const categories = [
        ...new Set(
            products
                .map(product =>
                    String(product.category || "").trim()
                )
                .filter(Boolean)
        )
    ].sort(
        (a, b) =>
            a.localeCompare(b)
    );


    /*
     * Reset category options.
     */

    categoryFilter.innerHTML = `
        <option value="">All Categories</option>
    `;


    /*
     * Add every unique category.
     */

    categories.forEach(category => {

        const option =
            document.createElement("option");

        option.value = category;
        option.textContent = category;

        categoryFilter.appendChild(option);
    });


    /*
     * Restore previously selected
     * category when possible.
     */

    if (
        currentValue &&
        categories.includes(currentValue)
    ) {
        categoryFilter.value =
            currentValue;
    }
}


/* =========================================================
   FILTER PRODUCTS
   ========================================================= */

function getFilteredProducts() {

    const searchTerm =
        searchInput
            ? searchInput.value
                .trim()
                .toLowerCase()
            : "";

    const selectedCategory =
        categoryFilter
            ? categoryFilter.value
            : "";


    return products.filter(product => {

        const name =
            String(
                product.name || ""
            ).toLowerCase();

        const description =
            String(
                product.description || ""
            ).toLowerCase();

        const category =
            String(
                product.category || ""
            ).toLowerCase();


        /*
         * Search checks:
         * - product name
         * - description
         * - category
         */

        const matchesSearch =
            !searchTerm ||
            name.includes(searchTerm) ||
            description.includes(searchTerm) ||
            category.includes(searchTerm);


        const matchesCategory =
            !selectedCategory ||
            product.category === selectedCategory;


        return (
            matchesSearch &&
            matchesCategory
        );
    });
}


/* =========================================================
   FORMAT PRICE
   ========================================================= */

function formatPrice(priceCents) {

    const value =
        Number(priceCents || 0) / 100;

    return new Intl.NumberFormat(
        "en-IN",
        {
            style: "currency",
            currency: "INR",
            maximumFractionDigits: 2
        }
    ).format(value);
}


/* =========================================================
   PRODUCT IMAGE
   ========================================================= */

function getProductImage(product) {

    if (product.image_url) {
        return product.image_url;
    }

    if (product.image) {
        return product.image;
    }

    /*
     * Neutral fallback image.
     */

    return "https://images.unsplash.com/photo-1523275335684-37898b6baf30?auto=format&fit=crop&w=600&q=80";
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


    /*
     * Product count.
     */

    if (productCount) {

        productCount.textContent =
            `${filteredProducts.length} ${
                filteredProducts.length === 1
                    ? "product"
                    : "products"
            }`;
    }


    /*
     * No products found.
     */

    if (filteredProducts.length === 0) {

        productGrid.innerHTML = `
            <div class="empty-state">
                <h3>No products found</h3>
                <p>
                    Try another search term
                    or category.
                </p>
            </div>
        `;

        return;
    }


    /*
     * Render product cards.
     */

    productGrid.innerHTML =
        filteredProducts
            .map(product => {

                const productId =
                    Number(product.id);

                const stock =
                    Number(
                        product.stock_qty ??
                        product.stock ??
                        0
                    );

                const image =
                    getProductImage(product);

                const category =
                    product.category ||
                    "General";

                const description =
                    product.description ||
                    "No description available.";


                const stockText =
                    stock > 0
                        ? `${stock} in stock`
                        : "Out of stock";


                const stockClass =
                    stock > 0
                        ? "in-stock"
                        : "out-of-stock";


                return `
    <article
        class="product-card"
        data-product-id="${productId}"
    >

        <img
            class="product-card-image"
            src="${escapeHtml(image)}"
            alt="${escapeHtml(product.name || "Product")}"
            loading="lazy"
            onerror="
                this.src='https://via.placeholder.com/600x400?text=KaniMart'
            "
        >

        <div class="product-card-body">

            <div class="product-card-category">
                ${escapeHtml(category)}
            </div>

            <h3 class="product-card-title">
                ${escapeHtml(product.name || "Unnamed Product")}
            </h3>

            <p class="product-card-description">
                ${escapeHtml(description)}
            </p>

            <div class="product-card-bottom">

                <strong class="product-card-price">
                    ${formatPrice(product.price_cents)}
                </strong>

                <span class="product-stock ${stockClass}">
                    ${escapeHtml(stockText)}
                </span>

            </div>

            <!-- REVIEW SUMMARY -->
            <div
                class="product-reviews"
                id="reviews-${productId}"
            >
                <span class="review-loading">
                    Loading reviews...
                </span>
            </div>

            <!-- ADD TO CART -->
            <button
                type="button"
                class="add-to-cart-button"
                data-product-id="${productId}"
                ${stock <= 0 ? "disabled" : ""}
            >
                🛒
                    ${
                        stock > 0
                            ? "Add to Cart"
                            : "Out of Stock"
                    }
                </button>

            </div>

        </article>
    `;
            })
            .join("");


    /*
     * Attach Add to Cart handlers.
     */

    productGrid
        .querySelectorAll(
            ".add-to-cart-button"
        )
        .forEach(button => {

            button.addEventListener(
                "click",
                async () => {

                    const productId =
                        Number(
                            button.dataset.productId
                        );

                    await addToCart(productId);
                }
            );
        });


    /*
     * Load reviews for every
     * displayed product.
     */

    filteredProducts.forEach(product => {

        loadReviews(
            Number(product.id)
        );
    });
}


/* =========================================================
   SEARCH
   ========================================================= */

if (searchInput) {

    searchInput.addEventListener(
        "input",
        () => {
            renderProducts();
        }
    );
}


/* =========================================================
   CATEGORY FILTER
   ========================================================= */

if (categoryFilter) {

    categoryFilter.addEventListener(
        "change",
        () => {
            renderProducts();
        }
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
        }
    );
}
/* =========================================================
   LOAD CART
   ========================================================= */

async function loadCart() {

    const user = getUser();

    if (!user || !user.id) {
        cart = {
            items: [],
            total_cents: 0
        };

        updateCartCount();
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

        if (!response.ok ||
            result.success === false) {

            throw new Error(
                result.error ||
                result.message ||
                "Unable to load cart."
            );
        }


        /*
         * Backend may return:
         *
         * result.data
         *
         * containing the cart object.
         */

        const data = result.data || {};


        cart = {
            items: Array.isArray(data.items)
                ? data.items
                : [],

            total_cents:
                Number(
                    data.total_cents || 0
                )
        };


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
   UPDATE CART COUNT
   ========================================================= */

function updateCartCount() {

    if (!cartCount) {
        return;
    }


    /*
     * Count total quantity rather than
     * number of different products.
     */

    const count =
        (cart.items || [])
            .reduce(
                (total, item) =>
                    total +
                    Number(
                        item.quantity || 0
                    ),
                0
            );


    cartCount.textContent =
        String(count);
}


/* =========================================================
   ADD PRODUCT TO CART
   ========================================================= */

async function addToCart(productId) {

    const user = getUser();

    if (!user || !user.id) {

        if (message) {
            message.textContent =
                "Please login to add products to your cart.";
        }

        showLogin();

        return;
    }


    const product =
        products.find(
            item =>
                Number(item.id) ===
                Number(productId)
        );


    if (!product) {

        console.error(
            "Product not found:",
            productId
        );

        return;
    }


    const stock =
        Number(
            product.stock_qty ??
            product.stock ??
            0
        );


    if (stock <= 0) {

        if (productMessage) {
            productMessage.textContent =
                "This product is currently out of stock.";
        }

        return;
    }


    /*
     * Disable the clicked button
     * while the request is running.
     */

    const button =
        productGrid
            ? productGrid.querySelector(
                `.add-to-cart-button[data-product-id="${productId}"]`
            )
            : null;


    const originalText =
        button
            ? button.textContent
            : "Add to Cart";


    if (button) {

        button.disabled = true;
        button.textContent =
            "Adding...";
    }


    try {

        const response = await fetch(
            `${API_BASE}/api/cart/${user.id}`,
            {
                method: "POST",

                headers: authHeaders(),

                body: JSON.stringify({
                    product_id:
                        Number(productId),

                    quantity: 1
                })
            }
        );


        const result =
            await response.json();


        if (!response.ok ||
            result.success === false) {

            throw new Error(
                result.error ||
                result.message ||
                "Unable to add product to cart."
            );
        }


        /*
         * Refresh the cart from
         * the backend so the count
         * is always accurate.
         */

        await loadCart();


        if (productMessage) {

            productMessage.textContent =
                `${product.name} added to your cart.`;
        }


        /*
         * Small temporary button
         * confirmation.
         */

        if (button) {

            button.textContent =
                "Added ✓";

            setTimeout(() => {

                if (button) {
                    button.textContent =
                        originalText;
                    button.disabled = false;
                }

            }, 900);
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


        if (button) {

            button.textContent =
                originalText;

            button.disabled = false;
        }
    }
}


/* =========================================================
   CART BUTTON
   ========================================================= */

if (cartButton) {

    cartButton.addEventListener(
        "click",
        () => {

            const user = getUser();

            if (!user || !getToken()) {

                showLogin();
                return;
            }

            window.location.href =
                "cart.html";
        }
    );
}


/* =========================================================
   CHECKOUT BUTTON
   ========================================================= */

if (checkoutButton) {

    checkoutButton.addEventListener(
        "click",
        () => {

            const user = getUser();

            if (!user || !getToken()) {

                showLogin();
                return;
            }

            window.location.href =
                "checkout.html";
        }
    );
}


/* =========================================================
   ORDER HISTORY BUTTON
   ========================================================= */

if (orderHistoryButton) {

    orderHistoryButton.addEventListener(
        "click",
        () => {

            const user = getUser();

            if (!user || !getToken()) {

                showLogin();
                return;
            }

            window.location.href =
                "orders.html";
        }
    );
}
/* =========================================================
   LOAD PRODUCT REVIEWS
   ========================================================= */

async function loadReviews(productId) {

    const reviewsContainer =
        document.getElementById(
            `reviews-${productId}`
        );

    if (!reviewsContainer) {
        return;
    }

    try {

        const response = await fetch(
            `${API_BASE}/api/products/${productId}/reviews`
        );

        const result = await response.json();

        if (!response.ok || result.success === false) {
            throw new Error(
                result.error ||
                result.message ||
                "Unable to load reviews."
            );
        }

        /*
         * Support both:
         *
         * data: [...]
         *
         * and:
         *
         * data: {
         *     reviews: [...]
         * }
         */

        const reviews =
            Array.isArray(result.data)
                ? result.data
                : Array.isArray(result.data?.reviews)
                    ? result.data.reviews
                    : [];

        /*
         * No reviews yet
         */

        if (reviews.length === 0) {

            reviewsContainer.innerHTML = `
                <div class="review-summary">

                    <span class="review-stars">
                        ☆☆☆☆☆
                    </span>

                    <span class="review-empty">
                        No reviews yet
                    </span>

                </div>
            `;

            return;
        }

        /*
         * Calculate average
         */

        const totalRating =
            reviews.reduce(
                (sum, review) =>
                    sum + Number(review.rating || 0),
                0
            );

        const averageRating =
            totalRating / reviews.length;

        const roundedRating =
            Math.round(averageRating);

        const stars =
            "★".repeat(
                Math.min(5, roundedRating)
            ) +
            "☆".repeat(
                Math.max(0, 5 - roundedRating)
            );

        /*
         * Display review summary
         */

        reviewsContainer.innerHTML = `
            <div class="review-summary">

                <span class="review-stars">
                    ${stars}
                </span>

                <span class="review-average">
                    ${averageRating.toFixed(1)}
                </span>

                <span class="review-count">
                    (${reviews.length} reviews)
                </span>

            </div>
        `;

    } catch (error) {

        console.error(
            "Review loading error:",
            error
        );

        /*
         * Do NOT show a scary error on every product card.
         * Simply show that reviews are currently unavailable.
         */

        reviewsContainer.innerHTML = `
            <span class="review-empty">
                ☆☆☆☆☆ Reviews unavailable
            </span>
        `;
    }
}


/* =========================================================
   SUBMIT REVIEW
   ========================================================= */

async function submitReview(productId) {

    const user = getUser();


    /*
     * User must be logged in.
     */

    if (!user || !user.id || !getToken()) {

        if (productMessage) {
            productMessage.textContent =
                "Please login before writing a review.";
        }

        showLogin();
        return;
    }


    /*
     * Find the selected product.
     */

    const product =
        products.find(
            item =>
                Number(item.id) ===
                Number(productId)
        );


    const productName =
        product
            ? product.name
            : "this product";


    /*
     * Ask for rating.
     *
     * Using prompt keeps the existing
     * marketplace simple and requires
     * no additional HTML.
     */

    const ratingInput =
        window.prompt(
            `Rate ${productName} from 1 to 5:`,
            "5"
        );


    /*
     * User cancelled.
     */

    if (ratingInput === null) {
        return;
    }


    const rating =
        Number(
            ratingInput.trim()
        );


    if (
        !Number.isInteger(rating) ||
        rating < 1 ||
        rating > 5
    ) {

        if (productMessage) {
            productMessage.textContent =
                "Rating must be a whole number from 1 to 5.";
        }

        return;
    }


    /*
     * Ask for review comment.
     */

    const commentInput =
        window.prompt(
            `Write your review for ${productName}:`,
            ""
        );


    /*
     * User cancelled.
     */

    if (commentInput === null) {
        return;
    }


    const comment =
        commentInput.trim();


    if (!comment) {

        if (productMessage) {
            productMessage.textContent =
                "Please enter a review comment.";
        }

        return;
    }


    if (comment.length > 1000) {

        if (productMessage) {
            productMessage.textContent =
                "Review must be 1000 characters or less.";
        }

        return;
    }


    /*
     * Submit review.
     *
     * Backend route:
     *
     * POST
     * /api/products/{productId}/reviews/{userId}
     */

    try {

        if (productMessage) {
            productMessage.textContent =
                "Submitting your review...";
        }


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


        const result =
            await response.json();


        if (!response.ok ||
            result.success === false) {

            throw new Error(
                result.error ||
                result.message ||
                "Unable to submit review."
            );
        }


        /*
         * Reload reviews so the new
         * rating appears immediately.
         */

        await loadReviews(productId);


        if (productMessage) {
            productMessage.textContent =
                "Review submitted successfully.";
        }


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
   SEARCH HANDLER
   ========================================================= */

if (searchInput) {

    searchInput.addEventListener(
        "input",
        () => {

            renderProducts();
        }
    );
}


/* =========================================================
   CATEGORY FILTER HANDLER
   ========================================================= */

if (categoryFilter) {

    categoryFilter.addEventListener(
        "change",
        () => {

            renderProducts();
        }
    );
}


/* =========================================================
   REFRESH PRODUCTS HANDLER
   ========================================================= */

if (refreshProducts) {

    refreshProducts.addEventListener(
        "click",
        async () => {

            if (productMessage) {
                productMessage.textContent =
                    "Refreshing products...";
            }

            await loadProducts();
            await loadCart();

            if (productMessage) {
                productMessage.textContent =
                    "Products refreshed.";
            }
        }
    );
}


/* =========================================================
   CART NAVIGATION
   ========================================================= */

if (cartButton) {

    cartButton.addEventListener(
        "click",
        async () => {

            const user = getUser();

            if (!user || !getToken()) {

                showLogin();
                return;
            }

            await loadCart();

            window.location.href =
                "cart.html";
        }
    );
}


/* =========================================================
   CHECKOUT NAVIGATION
   ========================================================= */

if (checkoutButton) {

    checkoutButton.addEventListener(
        "click",
        () => {

            const user = getUser();

            if (!user || !getToken()) {

                showLogin();
                return;
            }

            window.location.href =
                "checkout.html";
        }
    );
}


/* =========================================================
   ORDER HISTORY NAVIGATION
   ========================================================= */

if (orderHistoryButton) {

    orderHistoryButton.addEventListener(
        "click",
        () => {

            const user = getUser();

            if (!user || !getToken()) {

                showLogin();
                return;
            }

            window.location.href =
                "orders.html";
        }
    );
}


/* =========================================================
   INITIALIZE APPLICATION
   ========================================================= */

async function initializeApp() {

    const token =
        getToken();

    const user =
        getUser();


    /*
     * No saved login.
     */

    if (!token || !user) {

        showLogin();

        return;
    }


    /*
     * Existing login found.
     */

    try {

        await showMarketplace();

    } catch (error) {

        console.error(
            "Application initialization error:",
            error
        );


        /*
         * If initialization fails,
         * keep the user on the login screen.
         */

        localStorage.removeItem(
            "kanimart_token"
        );

        localStorage.removeItem(
            "kanimart_user"
        );

        showLogin();
    }
}


/* =========================================================
   START APPLICATION
   ========================================================= */

document.addEventListener(
    "DOMContentLoaded",
    () => {

        initializeApp();
    }
);