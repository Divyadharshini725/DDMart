// DDMart Application JavaScript Core
const DDMart = {
    currentUser: null,

    escapeHtml(str) {
        if (!str) return '';
        return String(str)
            .replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;')
            .replace(/"/g, '&quot;')
            .replace(/'/g, '&#039;');
    },

    showToast(message, type = 'info') {
        let container = document.getElementById('toast-container');
        if (!container) {
            container = document.createElement('div');
            container.id = 'toast-container';
            container.className = 'toast-container';
            document.body.appendChild(container);
        }
        const toast = document.createElement('div');
        toast.className = `toast toast-${type}`;
        toast.textContent = message;
        container.appendChild(toast);
        setTimeout(() => toast.remove(), 3500);
    },

    async checkAuth() {
        try {
            const res = await fetch('/api/v1/auth/me');
            const data = await res.json();
            if (data.success && data.data) {
                this.currentUser = data.data;
            } else {
                this.currentUser = null;
            }
        } catch (e) {
            this.currentUser = null;
        }
        this.renderNav();
        if (this.currentUser && this.currentUser.role === 'BUYER') {
            this.updateCartBadge();
        }
    },

    async logout() {
        try {
            await fetch('/api/v1/auth/logout', { method: 'POST' });
        } catch (e) {
            console.error(e);
        }
        this.currentUser = null;
        window.location.href = '/';
    },

    async updateCartBadge() {
        try {
            const res = await fetch('/api/v1/cart');
            const data = await res.json();
            if (data.success && data.data) {
                const badge = document.getElementById('cart-badge');
                if (badge) {
                    badge.textContent = data.data.total_items || 0;
                    badge.style.display = data.data.total_items > 0 ? 'inline-block' : 'none';
                }
            }
        } catch (e) {
            console.error(e);
        }
    },

    renderNav() {
        const navActions = document.getElementById('nav-actions');
        if (!navActions) return;

        if (!this.currentUser) {
            navActions.innerHTML = `
                <a href="/login.html" class="btn btn-outline">Log In</a>
                <a href="/register.html" class="btn btn-primary">Sign Up</a>
            `;
            return;
        }

        const u = this.currentUser;
        let links = '';
        if (u.role === 'BUYER') {
            links = `
                <a href="/cart.html" class="nav-link cart-btn">
                    🛒 Cart <span id="cart-badge" class="badge" style="display:none">0</span>
                </a>
                <a href="/orders.html" class="nav-link">📦 My Orders</a>
            `;
        } else if (u.role === 'SELLER') {
            links = `
                <a href="/seller.html" class="nav-link">📊 Seller Dashboard</a>
            `;
        } else if (u.role === 'ADMIN') {
            links = `
                <a href="/admin.html" class="nav-link">🛡️ Admin Panel</a>
                <a href="/orders.html" class="nav-link">📦 All Orders</a>
            `;
        }

        navActions.innerHTML = `
            ${links}
            <div style="display:flex; align-items:center; gap:0.5rem; margin-left:0.5rem;">
                <span style="font-size:0.9rem; font-weight:700; color:#4f46e5;">${this.escapeHtml(u.name)} (${this.escapeHtml(u.role)})</span>
                <button onclick="DDMart.logout()" class="btn btn-outline" style="padding:0.4rem 0.8rem; font-size:0.85rem;">Logout</button>
            </div>
        `;
    }
};

document.addEventListener('DOMContentLoaded', () => {
    DDMart.checkAuth();
});
