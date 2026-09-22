const API_BASE = window.KANIMART_API_BASE ||
    (window.location.protocol === "http:" &&
        (window.location.hostname === "127.0.0.1" ||
            window.location.hostname === "localhost")
        ? "http://127.0.0.1:8080"
        : "");


/* =========================================================
   ELEMENTS
   ========================================================= */

const productForm =
    document.getElementById("productForm");

const productId =
    document.getElementById("productId");

const productName =
    document.getElementById("productName");

const productImage =
    document.getElementById("productImage");

const productDescription =
    document.getElementById("productDescription");

const productPrice =
    document.getElementById("productPrice");

const productStock =
    document.getElementById("productStock");

const productCategory =
    document.getElementById("productCategory");

const productSubmitButton =
    document.getElementById(
        "productSubmitButton"
    );

const cancelEditButton =
    document.getElementById(
        "cancelEditButton"
    );

const sellerMessage =
    document.getElementById(
        "sellerMessage"
    );

const sellerProducts =
    document.getElementById(
        "sellerProducts"
    );

const refreshSellerProducts =
    document.getElementById(
        "refreshSellerProducts"
    );

const logoutButton =
    document.getElementById(
        "logoutButton"
    );

const formTitle =
    document.getElementById(
        "formTitle"
    );

const editBadge =
    document.getElementById(
        "editBadge"
    );

const imagePreview =
    document.getElementById(
        "imagePreview"
    );

const imagePreviewContainer =
    document.getElementById(
        "imagePreviewContainer"
    );

const sellerOrders =
    document.getElementById("sellerOrders");

const refreshSellerOrders =
    document.getElementById("refreshSellerOrders");

const sellerOrderMessage =
    document.getElementById("sellerOrderMessage");


let products = [];

let sellerOrderData = [];


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
   AUTHORIZATION CHECK
   ========================================================= */

function checkSellerAccess() {

    const token =
        getToken();

    const user =
        getUser();

    if (!token || !user) {

        alert(
            "Please login before accessing Seller Management."
        );

        window.location.href =
            "index.html";

        return false;
    }


    if (
        user.role !== "SELLER" &&
        user.role !== "ADMIN"
    ) {

        alert(
            "Seller Management is available only to sellers and administrators."
        );

        window.location.href =
            "index.html";

        return false;
    }

    return true;
}


/* =========================================================
   HTML ESCAPING
   ========================================================= */

function escapeHtml(value) {

    return String(value)
        .replaceAll("&", "&amp;")
        .replaceAll("<", "&lt;")
        .replaceAll(">", "&gt;")
        .replaceAll('"', "&quot;")
        .replaceAll("'", "&#039;");
}


/* =========================================================
   IMAGE URL
   ========================================================= */

function getImageUrl(product) {

    if (
        product &&
        product.image_url &&
        String(product.image_url).trim()
    ) {
        return String(
            product.image_url
        ).trim();
    }

    return "https://via.placeholder.com/600x400?text=KaniMart";
}

function getOrderStatusOptions(status) {
    const transitions = {
        PENDING: ["CONFIRMED", "CANCELLED"],
        CONFIRMED: ["SHIPPED", "CANCELLED"],
        SHIPPED: ["DELIVERED"],
        DELIVERED: [],
        CANCELLED: []
    };

    return [status, ...(transitions[status] || [])]
        .filter((value, index, values) => values.indexOf(value) === index);
}

function formatOrderDate(value) {
    if (!value) {
        return "Date unavailable";
    }

    const date = new Date(value);
    return Number.isNaN(date.getTime())
        ? String(value)
        : date.toLocaleDateString("en-IN", {
            day: "2-digit",
            month: "short",
            year: "numeric"
        });
}

async function loadSellerOrders() {
    if (!sellerOrders || !checkSellerAccess()) {
        return;
    }

    sellerOrders.innerHTML = `<div class="loading">Loading orders...</div>`;

    try {
        const response = await fetch(
            `${API_BASE}/api/seller/orders`,
            { headers: authHeaders() }
        );
        const result = await response.json();

        if (!response.ok) {
            throw new Error(result.message || "Unable to load orders.");
        }

        sellerOrderData = result.data?.orders || [];
        renderSellerOrders();
    } catch (error) {
        sellerOrders.innerHTML = `
            <div class="empty-state">
                <h3>Unable to load orders</h3>
                <p>${escapeHtml(error.message || "Please try again.")}</p>
            </div>
        `;
    }
}

function renderSellerOrders() {
    if (!sellerOrderData.length) {
        sellerOrders.innerHTML = `
            <div class="empty-state">
                <h3>No orders to fulfill</h3>
                <p>Orders containing your products will appear here.</p>
            </div>
        `;
        return;
    }

    sellerOrders.innerHTML = sellerOrderData.map(order => {
        const status = order.status || "PENDING";
        const options = getOrderStatusOptions(status)
            .map(option => `
                <option value="${option}" ${option === status ? "selected" : ""}>
                    ${option.charAt(0) + option.slice(1).toLowerCase()}
                </option>
            `)
            .join("");
        const total = Number(order.total_amount_cents || 0) / 100;

        return `
            <article class="seller-order">
                <div class="seller-order-id">
                    <strong>Order #${Number(order.id)}</strong>
                    <span>${formatOrderDate(order.created_at)}</span>
                </div>
                <div class="seller-order-buyer">
                    <strong>Customer</strong>
                    <span>${escapeHtml(order.buyer_email || "Customer")}</span>
                </div>
                <div class="seller-order-total">
                    <strong>₹${total.toFixed(2)}</strong>
                    <span>Order total</span>
                </div>
                <select class="seller-order-status" data-order-id="${Number(order.id)}" aria-label="Update order ${Number(order.id)} status">
                    ${options}
                </select>
            </article>
        `;
    }).join("");

    sellerOrders.querySelectorAll(".seller-order-status").forEach(select => {
        select.addEventListener("change", () => updateSellerOrderStatus(select));
    });
}

async function updateSellerOrderStatus(select) {
    const user = getUser();
    const orderId = Number(select.dataset.orderId);
    const newStatus = select.value;
    const order = sellerOrderData.find(item => Number(item.id) === orderId);

    if (!user || !order) {
        return;
    }

    select.disabled = true;
    sellerOrderMessage.textContent = `Updating order #${orderId}...`;

    try {
        const response = await fetch(
            `${API_BASE}/api/orders/${user.id}/${orderId}/status`,
            {
                method: "PUT",
                headers: authHeaders(),
                body: JSON.stringify({ status: newStatus })
            }
        );
        const result = await response.json();

        if (!response.ok) {
            throw new Error(result.message || "Unable to update order status.");
        }

        sellerOrderMessage.textContent = `Order #${orderId} is now ${newStatus}.`;
        await loadSellerOrders();
    } catch (error) {
        sellerOrderMessage.textContent = error.message || "Unable to update order status.";
        renderSellerOrders();
    }
}


/* =========================================================
   IMAGE PREVIEW
   ========================================================= */

function updateImagePreview() {

    if (!imagePreview ||
        !imagePreviewContainer) {
        return;
    }

    const url =
        productImage.value.trim();

    if (!url) {

        imagePreviewContainer.classList
            .remove("visible");

        imagePreview.removeAttribute(
            "src"
        );

        return;
    }

    imagePreview.src = url;

    imagePreviewContainer.classList
        .add("visible");
}


productImage.addEventListener(
    "input",
    updateImagePreview
);


imagePreview.addEventListener(
    "error",
    () => {

        imagePreviewContainer.classList
            .add("visible");

        imagePreview.src =
            "https://via.placeholder.com/180x120?text=Invalid+Image";
    }
);


/* =========================================================
   LOAD PRODUCTS
   ========================================================= */

async function loadSellerProducts() {

    if (!checkSellerAccess()) {
        return;
    }

    sellerProducts.innerHTML = `
        <div class="loading">
            Loading your products...
        </div>
    `;

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


        const allProducts =
            Array.isArray(result)
                ? result
                : Array.isArray(result.data)
                    ? result.data
                    : [];


        const user =
            getUser();


        /*
         * Seller sees only products belonging
         * to their authenticated seller ID.
         *
         * Admin can see all products.
         */
        if (user.role === "ADMIN") {

            products =
                allProducts;

        } else {

            products =
                allProducts.filter(
                    product =>
                        Number(
                            product.seller_id
                        ) ===
                        Number(user.id)
                );
        }


        renderSellerProducts();

    } catch (error) {

        console.error(error);

        sellerProducts.innerHTML = `
            <div class="empty-state">
                <h3>Unable to load products</h3>
                <p>
                    ${escapeHtml(
                        error.message ||
                        "Please try again."
                    )}
                </p>
            </div>
        `;
    }
}


/* =========================================================
   RENDER PRODUCTS
   ========================================================= */

function renderSellerProducts() {

    if (!products.length) {

        sellerProducts.innerHTML = `
            <div class="empty-state">
                <h3>No products yet</h3>
                <p>
                    Create your first product using
                    the form above.
                </p>
            </div>
        `;

        return;
    }


    sellerProducts.innerHTML =
        products
            .map(product => {

                const price =
                    Number(
                        product.price_cents || 0
                    ) / 100;

                const stock =
                    Number(
                        product.stock_qty || 0
                    );

                const imageUrl =
                    getImageUrl(product);


                return `
                    <article
                        class="seller-product"
                    >

                        <img
                            class="product-image"
                            src="${escapeHtml(
                                imageUrl
                            )}"
                            alt="${escapeHtml(
                                product.name ||
                                "Product"
                            )}"
                            onerror="
                                this.src='https://via.placeholder.com/600x400?text=KaniMart'
                            "
                        >


                        <div
                            class="product-content"
                        >

                            <span
                                class="product-id"
                            >
                                Product ID:
                                #${Number(
                                    product.id
                                )}
                            </span>


                            <h3>
                                ${escapeHtml(
                                    product.name ||
                                    "Unnamed Product"
                                )}
                            </h3>


                            <p
                                class="product-description"
                            >
                                ${escapeHtml(
                                    product.description ||
                                    "No description available."
                                )}
                            </p>


                            <div
                                class="product-details"
                            >

                                <div
                                    class="detail-box"
                                >

                                    <span
                                        class="detail-label"
                                    >
                                        Price
                                    </span>

                                    <span
                                        class="detail-value"
                                    >
                                        ₹${price.toLocaleString(
                                            "en-IN",
                                            {
                                                minimumFractionDigits:
                                                    2
                                            }
                                        )}
                                    </span>

                                </div>


                                <div
                                    class="detail-box"
                                >

                                    <span
                                        class="detail-label"
                                    >
                                        Stock
                                    </span>

                                    <span
                                        class="detail-value"
                                    >
                                        ${stock}
                                    </span>

                                </div>


                                <div
                                    class="detail-box"
                                >

                                    <span
                                        class="detail-label"
                                    >
                                        Category
                                    </span>

                                    <span
                                        class="detail-value"
                                    >
                                        ${escapeHtml(
                                            product.category ||
                                            "General"
                                        )}
                                    </span>

                                </div>


                                <div
                                    class="detail-box"
                                >

                                    <span
                                        class="detail-label"
                                    >
                                        Seller ID
                                    </span>

                                    <span
                                        class="detail-value"
                                    >
                                        ${Number(
                                            product.seller_id ||
                                            0
                                        )}
                                    </span>

                                </div>

                            </div>


                            <div
                                class="product-actions"
                            >

                                <button
                                    type="button"
                                    class="primary-button"
                                    data-edit-product="${Number(
                                        product.id
                                    )}"
                                >
                                    Edit
                                </button>


                                <button
                                    type="button"
                                    class="danger-button"
                                    data-delete-product="${Number(
                                        product.id
                                    )}"
                                >
                                    Delete
                                </button>

                            </div>

                        </div>

                    </article>
                `;
            })
            .join("");


    /*
     * Edit buttons
     */
    sellerProducts
        .querySelectorAll(
            "[data-edit-product]"
        )
        .forEach(button => {

            button.addEventListener(
                "click",
                () => {

                    const id =
                        Number(
                            button.dataset
                                .editProduct
                        );

                    startEditProduct(id);
                }
            );
        });


    /*
     * Delete buttons
     */
    sellerProducts
        .querySelectorAll(
            "[data-delete-product]"
        )
        .forEach(button => {

            button.addEventListener(
                "click",
                () => {

                    const id =
                        Number(
                            button.dataset
                                .deleteProduct
                        );

                    deleteProduct(id);
                }
            );
        });
}


/* =========================================================
   START EDIT
   ========================================================= */

function startEditProduct(
    id
) {

    const product =
        products.find(
            item =>
                Number(item.id) ===
                Number(id)
        );


    if (!product) {

        sellerMessage.textContent =
            "Product not found.";

        return;
    }


    /*
     * Product ID is displayed
     * but remains readonly.
     */
    productId.value =
        product.id;


    productName.value =
        product.name || "";


    productImage.value =
        product.image_url || "";


    productDescription.value =
        product.description || "";


    productPrice.value =
        (
            Number(
                product.price_cents || 0
            ) / 100
        ).toFixed(2);


    productStock.value =
        Number(
            product.stock_qty || 0
        );


    productCategory.value =
        product.category || "";


    productSubmitButton.textContent =
        "Update Product";


    cancelEditButton.hidden =
        false;


    formTitle.textContent =
        "Edit Product";


    editBadge.classList.add(
        "visible"
    );


    sellerMessage.textContent =
        `Editing Product #${product.id}`;


    updateImagePreview();


    /*
     * Scroll back to the form.
     */
    productForm.scrollIntoView({
        behavior: "smooth",
        block: "start"
    });
}


/* =========================================================
   RESET FORM
   ========================================================= */

function resetProductForm() {

    productForm.reset();


    /*
     * Clear Product ID.
     */
    productId.value = "";


    productSubmitButton.textContent =
        "Create Product";


    cancelEditButton.hidden =
        true;


    formTitle.textContent =
        "Add New Product";


    editBadge.classList.remove(
        "visible"
    );


    imagePreviewContainer.classList
        .remove("visible");


    imagePreview.removeAttribute(
        "src"
    );
}


/* =========================================================
   CREATE / UPDATE PRODUCT
   ========================================================= */

productForm.addEventListener(
    "submit",
    async event => {

        event.preventDefault();


        if (!checkSellerAccess()) {
            return;
        }


        const name =
            productName.value.trim();


        const imageUrl =
            productImage.value.trim();


        const description =
            productDescription.value.trim();


        const price =
            Number(
                productPrice.value
            );


        const stock =
            Number(
                productStock.value
            );


        const category =
            productCategory.value.trim();


        const editingId =
            productId.value.trim();


        const isEditing =
            Boolean(editingId);


        /*
         * Validation
         */
        if (!name) {

            sellerMessage.textContent =
                "Please enter a product name.";

            productName.focus();

            return;
        }


        if (!description) {

            sellerMessage.textContent =
                "Please enter a product description.";

            productDescription.focus();

            return;
        }


        if (
            !Number.isFinite(price) ||
            price < 0
        ) {

            sellerMessage.textContent =
                "Please enter a valid price.";

            productPrice.focus();

            return;
        }


        if (
            !Number.isInteger(stock) ||
            stock < 0
        ) {

            sellerMessage.textContent =
                "Stock must be a whole number greater than or equal to 0.";

            productStock.focus();

            return;
        }


        if (!category) {

            sellerMessage.textContent =
                "Please enter a category.";

            productCategory.focus();

            return;
        }


        /*
         * URL validation.
         *
         * Empty image URL is allowed.
         */
        if (imageUrl) {

            try {

                new URL(imageUrl);

            } catch {

                sellerMessage.textContent =
                    "Please enter a valid image URL.";

                productImage.focus();

                return;
            }
        }


        const url =
            isEditing
                ? `${API_BASE}/api/products/${editingId}`
                : `${API_BASE}/api/products`;


        const method =
            isEditing
                ? "PUT"
                : "POST";


        sellerMessage.textContent =
            isEditing
                ? "Updating product..."
                : "Creating product...";


        productSubmitButton.disabled =
            true;


        try {

            /*
             * IMPORTANT:
             *
             * seller_id is intentionally NOT
             * sent from the browser.
             *
             * The backend gets seller_id
             * from the authenticated JWT.
             */
            const body = {

                name,

                description,

                price_cents:
                    Math.round(
                        price * 100
                    ),

                stock_qty:
                    stock,

                category,

                image_url:
                    imageUrl || null
            };


            const response =
                await fetch(
                    url,
                    {
                        method,

                        headers:
                            authHeaders(),

                        body:
                            JSON.stringify(
                                body
                            )
                    }
                );


            const result =
                await response.json();


            if (
                !response.ok ||
                result.success === false
            ) {

                throw new Error(
                    result.error ||
                    result.message ||
                    "Product operation failed."
                );
            }


            sellerMessage.textContent =
                isEditing
                    ? `Product #${editingId} updated successfully.`
                    : "Product created successfully.";


            resetProductForm();


            await loadSellerProducts();


        } catch (error) {

            console.error(
                "Product save error:",
                error
            );


            sellerMessage.textContent =
                error.message ||
                "Unable to save product.";


        } finally {

            productSubmitButton.disabled =
                false;
        }
    }
);


/* =========================================================
   CANCEL EDIT
   ========================================================= */

cancelEditButton.addEventListener(
    "click",
    () => {

        resetProductForm();

        sellerMessage.textContent =
            "";
    }
);


/* =========================================================
   DELETE PRODUCT
   ========================================================= */

async function deleteProduct(
    id
) {

    const product =
        products.find(
            item =>
                Number(item.id) ===
                Number(id)
        );


    const productNameText =
        product?.name ||
        `Product #${id}`;


    const confirmed =
        window.confirm(
            `Delete "${productNameText}" (Product #${id})?\n\nThis action cannot be undone.`
        );


    if (!confirmed) {
        return;
    }


    sellerMessage.textContent =
        `Deleting Product #${id}...`;


    try {

        const response =
            await fetch(
                `${API_BASE}/api/products/${id}`,
                {
                    method: "DELETE",
                    headers: authHeaders()
                }
            );


        const result =
            await response.json();


        if (
            !response.ok ||
            result.success === false
        ) {

            throw new Error(
                result.error ||
                result.message ||
                "Unable to delete product."
            );
        }


        sellerMessage.textContent =
            `Product #${id} deleted successfully.`;


        /*
         * If the deleted product was
         * currently being edited,
         * clear the form.
         */
        if (
            Number(productId.value) ===
            Number(id)
        ) {
            resetProductForm();
        }


        await loadSellerProducts();


    } catch (error) {

        console.error(
            "Delete product error:",
            error
        );


        sellerMessage.textContent =
            error.message ||
            "Unable to delete product.";
    }
}


/* =========================================================
   REFRESH
   ========================================================= */

refreshSellerProducts.addEventListener(
    "click",
    async () => {

        await loadSellerProducts();

    }
);

refreshSellerOrders.addEventListener(
    "click",
    loadSellerOrders
);


/* =========================================================
   LOGOUT
   ========================================================= */

logoutButton.addEventListener(
    "click",
    () => {

        localStorage.removeItem(
            "kanimart_token"
        );

        localStorage.removeItem(
            "kanimart_user"
        );

        window.location.href =
            "index.html";
    }
);


/* =========================================================
   INITIALIZATION
   ========================================================= */

document.addEventListener(
    "DOMContentLoaded",
    () => {

        if (!checkSellerAccess()) {
            return;
        }

        loadSellerProducts();
        loadSellerOrders();
    }
);