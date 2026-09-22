const API_BASE = window.KANIMART_API_BASE ||
    (window.location.protocol === "http:" &&
        (window.location.hostname === "127.0.0.1" ||
            window.location.hostname === "localhost")
        ? "http://127.0.0.1:8080"
        : "");

const token =
    localStorage.getItem("kanimart_token");

const user = JSON.parse(
    localStorage.getItem("kanimart_user") || "null"
);


/* --------------------------------
   Authentication
-------------------------------- */

if (!token || !user) {
    window.location.href = "index.html";
}


/* --------------------------------
   Elements
-------------------------------- */

const checkoutSummary =
    document.getElementById("checkoutSummary");

const subtotalElement =
    document.getElementById("subtotal");

const totalElement =
    document.getElementById("checkoutTotal");

const placeOrderButton =
    document.getElementById("placeOrderButton");

const checkoutMessage =
    document.getElementById("checkoutMessage");

const paymentDetails =
    document.getElementById("paymentDetails");


/* --------------------------------
   Payment Details Templates
-------------------------------- */

const paymentTemplates = {

    card: `
        <h4>Card Details</h4>

        <div class="field-group">
            <label for="cardName">
                Cardholder Name
            </label>

            <input
                id="cardName"
                type="text"
                placeholder="Enter cardholder name"
                autocomplete="off"
            >
        </div>

        <div class="field-group">
            <label for="cardNumber">
                Card Number
            </label>

            <input
                id="cardNumber"
                type="text"
                inputmode="numeric"
                maxlength="19"
                placeholder="1234 5678 9012 3456"
                autocomplete="off"
            >
        </div>

        <div class="field-row">

            <div class="field-group">
                <label for="cardExpiry">
                    Expiry Date
                </label>

                <input
                    id="cardExpiry"
                    type="text"
                    maxlength="5"
                    placeholder="MM/YY"
                    autocomplete="off"
                >
            </div>

            <div class="field-group">
                <label for="cardCvv">
                    CVV
                </label>

                <input
                    id="cardCvv"
                    type="password"
                    inputmode="numeric"
                    maxlength="3"
                    placeholder="123"
                    autocomplete="off"
                >
            </div>

        </div>
    `,


    upi: `
        <h4>UPI Payment</h4>

        <div class="field-group">
            <label for="upiId">
                UPI ID
            </label>

            <input
                id="upiId"
                type="text"
                placeholder="example@upi"
                autocomplete="off"
            >
        </div>

        <div class="payment-description">
            Enter a demo UPI ID such as
            <strong>student@upi</strong>.
        </div>
    `,


    netbanking: `
        <h4>Net Banking</h4>

        <div class="field-group">
            <label for="bank">
                Select Bank
            </label>

            <select id="bank">

                <option value="">
                    Select your bank
                </option>

                <option value="sbi">
                    State Bank of India
                </option>

                <option value="hdfc">
                    HDFC Bank
                </option>

                <option value="icici">
                    ICICI Bank
                </option>

                <option value="axis">
                    Axis Bank
                </option>

                <option value="kotak">
                    Kotak Mahindra Bank
                </option>

                <option value="other">
                    Other Bank
                </option>

            </select>
        </div>
    `,


    wallet: `
        <h4>Digital Wallet</h4>

        <div class="field-group">
            <label for="wallet">
                Select Wallet
            </label>

            <select id="wallet">

                <option value="">
                    Select wallet
                </option>

                <option value="paytm">
                    Paytm
                </option>

                <option value="phonepe">
                    PhonePe
                </option>

                <option value="amazonpay">
                    Amazon Pay
                </option>

                <option value="mobikwik">
                    MobiKwik
                </option>

            </select>
        </div>
    `,


    cod: `
        <h4>Cash on Delivery</h4>

        <p style="margin:0;color:#6b7280;font-size:14px;">
            You will pay the order amount when the
            delivery arrives.
        </p>

        <div
            style="
                margin-top:12px;
                padding:10px;
                border-radius:8px;
                background:#ecfdf5;
                color:#047857;
                font-size:13px;
            "
        >
            ✓ No online payment required.
        </div>
    `
};


/* --------------------------------
   Show Payment Details
-------------------------------- */

function showPaymentDetails(method) {

    paymentDetails.innerHTML =
        paymentTemplates[method] ||
        "";

}


/* --------------------------------
   Payment Method Selection
-------------------------------- */

document
    .querySelectorAll(
        'input[name="paymentMethod"]'
    )
    .forEach(input => {

        input.addEventListener(
            "change",
            () => {

                showPaymentDetails(
                    input.value
                );

            }
        );

    });


/* --------------------------------
   Card Number Formatting
-------------------------------- */

document.addEventListener(
    "input",
    event => {

        if (
            event.target.id ===
            "cardNumber"
        ) {

            let value =
                event.target.value
                    .replace(/\D/g, "")
                    .substring(0, 16);

            value =
                value.match(/.{1,4}/g)
                    ?.join(" ") || "";

            event.target.value =
                value;
        }


        if (
            event.target.id ===
            "cardExpiry"
        ) {

            let value =
                event.target.value
                    .replace(/\D/g, "")
                    .substring(0, 4);

            if (value.length >= 3) {

                value =
                    value.substring(0, 2) +
                    "/" +
                    value.substring(2);

            }

            event.target.value =
                value;
        }


        if (
            event.target.id ===
            "cardCvv"
        ) {

            event.target.value =
                event.target.value
                    .replace(/\D/g, "")
                    .substring(0, 3);

        }

    }
);


/* --------------------------------
   Validate Payment Method
-------------------------------- */

function validatePaymentMethod() {

    const selected =
        document.querySelector(
            'input[name="paymentMethod"]:checked'
        );


    if (!selected) {

        return {
            valid: false,
            message:
                "Please select a payment method."
        };

    }


    const method =
        selected.value;


    if (method === "card") {

        const name =
            document.getElementById(
                "cardName"
            )?.value.trim();

        const number =
            document.getElementById(
                "cardNumber"
            )?.value.replace(/\s/g, "");

        const expiry =
            document.getElementById(
                "cardExpiry"
            )?.value.trim();

        const cvv =
            document.getElementById(
                "cardCvv"
            )?.value.trim();


        if (!name) {

            return {
                valid: false,
                message:
                    "Please enter the cardholder name."
            };

        }


        if (!/^\d{16}$/.test(number)) {

            return {
                valid: false,
                message:
                    "Please enter a valid 16-digit card number."
            };

        }


        if (!/^\d{2}\/\d{2}$/.test(expiry)) {

            return {
                valid: false,
                message:
                    "Please enter expiry in MM/YY format."
            };

        }


        if (!/^\d{3}$/.test(cvv)) {

            return {
                valid: false,
                message:
                    "Please enter a valid 3-digit CVV."
            };

        }

    }


    if (method === "upi") {

        const upi =
            document.getElementById(
                "upiId"
            )?.value.trim();


        if (
            !upi ||
            !/^[\w.-]+@[\w.-]+$/.test(upi)
        ) {

            return {
                valid: false,
                message:
                    "Please enter a valid UPI ID."
            };

        }

    }


    if (method === "netbanking") {

        const bank =
            document.getElementById(
                "bank"
            )?.value;


        if (!bank) {

            return {
                valid: false,
                message:
                    "Please select your bank."
            };

        }

    }


    if (method === "wallet") {

        const wallet =
            document.getElementById(
                "wallet"
            )?.value;


        if (!wallet) {

            return {
                valid: false,
                message:
                    "Please select a wallet."
            };

        }

    }


    return {
        valid: true,
        method
    };

}


/* --------------------------------
   Load Checkout Summary
-------------------------------- */

async function loadCheckoutSummary() {

    try {

        const response =
            await fetch(
                `${API_BASE}/api/cart/${user.id}`,
                {
                    method: "GET",

                    headers: {
                        "Authorization":
                            `Bearer ${token}`,

                        "Content-Type":
                            "application/json"
                    }
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
                "Unable to load cart"
            );

        }


        let items = [];


        if (
            Array.isArray(result.data)
        ) {

            items =
                result.data;

        }
        else if (
            result.data &&
            Array.isArray(
                result.data.items
            )
        ) {

            items =
                result.data.items;

        }


        if (items.length === 0) {

            checkoutSummary.innerHTML = `
                <div class="empty-checkout">

                    <div class="empty-checkout-icon">
                        🛒
                    </div>

                    <h3>
                        Your cart is empty
                    </h3>

                    <p>
                        Add products to your cart
                        before checking out.
                    </p>

                    <a href="index.html">
                        Continue Shopping
                    </a>

                </div>
            `;

            totalElement.textContent =
                "0.00";

            subtotalElement.textContent =
                "0.00";

            placeOrderButton.disabled =
                true;

            return;

        }


        let total = 0;


        checkoutSummary.innerHTML =
            items.map(item => {

                const price =
                    Number(
                        item.price_cents || 0
                    ) / 100;

                const quantity =
                    Number(
                        item.quantity || 0
                    );

                const itemSubtotal =
                    price * quantity;

                total += itemSubtotal;


                return `
                    <div class="checkout-item">

                        <div>

                            <strong>
                                ${item.name || "Product"}
                            </strong>

                            <p>
                                ₹${price.toFixed(2)}
                                ×
                                ${quantity}
                            </p>

                        </div>

                        <strong>
                            ₹${itemSubtotal.toFixed(2)}
                        </strong>

                    </div>
                `;

            }).join("");


        totalElement.textContent =
            total.toFixed(2);

        subtotalElement.textContent =
            total.toFixed(2);

    }
    catch (error) {

        console.error(
            "Checkout loading error:",
            error
        );


        checkoutSummary.innerHTML = `
            <div class="error-message">
                Unable to load checkout:
                ${error.message}
            </div>
        `;

        placeOrderButton.disabled =
            true;

    }

}


/* --------------------------------
   Place Order
-------------------------------- */

placeOrderButton.addEventListener(
    "click",
    async () => {

        const validation =
            validatePaymentMethod();


        if (!validation.valid) {

            checkoutMessage.style.background =
                "#fef2f2";

            checkoutMessage.style.color =
                "#b91c1c";

            checkoutMessage.style.border =
                "1px solid #fecaca";

            checkoutMessage.textContent =
                validation.message;

            return;

        }


        placeOrderButton.disabled =
            true;

        placeOrderButton.textContent =
            "Processing Payment...";


        checkoutMessage.textContent =
            "";


        try {

            /*
             * Mock payment processing.
             *
             * The selected method is intentionally
             * handled only on the frontend because
             * the current backend checkout endpoint
             * does not require a payment_method field.
             */

            await new Promise(
                resolve =>
                    setTimeout(resolve, 900)
            );


            const response =
                await fetch(
                    `${API_BASE}/api/orders/checkout/${user.id}`,
                    {
                        method: "POST",

                        headers: {
                            "Authorization":
                                `Bearer ${token}`,

                            "Content-Type":
                                "application/json"
                        }
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
                    "Checkout failed"
                );

            }


            const orderId =
                result.data?.id ||
                result.data?.order_id ||
                result.order_id;


            const methodNames = {
                card:
                    "Credit / Debit Card",
                upi:
                    "UPI",
                netbanking:
                    "Net Banking",
                wallet:
                    "Digital Wallet",
                cod:
                    "Cash on Delivery"
            };


            const selectedMethod =
                methodNames[
                    validation.method
                ];


            checkoutMessage.style.background =
                "#ecfdf5";

            checkoutMessage.style.color =
                "#047857";

            checkoutMessage.style.border =
                "1px solid #a7f3d0";


            checkoutMessage.textContent =
                orderId
                    ? `✓ Payment successful via ${selectedMethod}. Order #${orderId} placed!`
                    : `✓ Payment successful via ${selectedMethod}. Order placed successfully!`;


            placeOrderButton.textContent =
                "Order Placed ✓";


            setTimeout(
                () => {

                    window.location.href =
                        "orders.html";

                },
                1800
            );

        }
        catch (error) {

            console.error(
                "Checkout error:",
                error
            );


            checkoutMessage.style.background =
                "#fef2f2";

            checkoutMessage.style.color =
                "#b91c1c";

            checkoutMessage.style.border =
                "1px solid #fecaca";


            checkoutMessage.textContent =
                `Checkout failed: ${error.message}`;


            placeOrderButton.disabled =
                false;

            placeOrderButton.textContent =
                "Place Order";

        }

    }
);


/* --------------------------------
   Start
-------------------------------- */

showPaymentDetails("card");

loadCheckoutSummary();