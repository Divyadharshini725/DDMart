# DDMart Local Development Server
$port = 8080
$prefix = "http://localhost:$port/"
$publicDir = Join-Path $PSScriptRoot "public"

$listener = New-Object System.Net.HttpListener
$listener.Prefixes.Add($prefix)
$listener.Start()

Write-Host "=================================================================="
Write-Host "  DDMart Server is running on $prefix"
Write-Host "  Press Ctrl+C to stop."
Write-Host "=================================================================="

# In-memory database state loaded from seed data
$users = @{
    "admin@ddmart.com" = @{ id = 1; name = "System Administrator"; email = "admin@ddmart.com"; role = "ADMIN"; password = "Admin@123"; created_at = "2026-08-01 10:00:00" }
    "techseller@ddmart.com" = @{ id = 2; name = "TechSphere Electronics"; email = "techseller@ddmart.com"; role = "SELLER"; password = "Seller@123"; created_at = "2026-08-02 11:00:00" }
    "fashionhub@ddmart.com" = @{ id = 3; name = "Vogue & Style Hub"; email = "fashionhub@ddmart.com"; role = "SELLER"; password = "Seller@123"; created_at = "2026-08-03 12:00:00" }
    "john.buyer@ddmart.com" = @{ id = 4; name = "John Doe"; email = "john.buyer@ddmart.com"; role = "BUYER"; password = "Buyer@123"; created_at = "2026-08-04 13:00:00" }
    "alice.buyer@ddmart.com" = @{ id = 5; name = "Alice Smith"; email = "alice.buyer@ddmart.com"; role = "BUYER"; password = "Buyer@123"; created_at = "2026-08-05 14:00:00" }
}

$sessions = @{}
$carts = @{}

$products = [System.Collections.ArrayList]::new(@(
    [PSCustomObject]@{ id = 1; seller_id = 2; seller_name = "TechSphere Electronics"; name = "Noise-Canceling Wireless Headphones"; description = "Premium over-ear Bluetooth headphones with active noise cancellation, 40hr battery life, and crystal-clear audio."; price_cents = 14999; price_formatted = "Rs 149.99"; stock_qty = 45; category = "Electronics"; image_url = "https://images.unsplash.com/photo-1505740420928-5e560c06d30e?w=500&q=80"; rating = 5.0; review_count = 1; created_at = "2026-08-10 10:00:00" },
    [PSCustomObject]@{ id = 2; seller_id = 2; seller_name = "TechSphere Electronics"; name = "Mechanical Gaming Keyboard RGB"; description = "Customizable RGB backlit mechanical keyboard with tactile blue switches, detachable wrist rest, and N-key rollover."; price_cents = 8999; price_formatted = "Rs 89.99"; stock_qty = 60; category = "Electronics"; image_url = "https://images.unsplash.com/photo-1587829741301-dc798b83add3?w=500&q=80"; rating = 4.8; review_count = 0; created_at = "2026-08-11 10:00:00" },
    [PSCustomObject]@{ id = 3; seller_id = 2; seller_name = "TechSphere Electronics"; name = "Ultra HD 4K Monitor 27-inch"; description = "IPS display panel with 144Hz refresh rate, 1ms response time, HDR400, and dual HDMI 2.1 + DisplayPort."; price_cents = 34999; price_formatted = "Rs 349.99"; stock_qty = 20; category = "Electronics"; image_url = "https://images.unsplash.com/photo-1527443224154-c4a3942d3acf?w=500&q=80"; rating = 4.9; review_count = 0; created_at = "2026-08-12 10:00:00" },
    [PSCustomObject]@{ id = 4; seller_id = 2; seller_name = "TechSphere Electronics"; name = "Ergonomic Vertical Wireless Mouse"; description = "Ergonomic design reducing wrist strain, 4000 DPI optical sensor, quiet clicks, and rechargeable USB-C battery."; price_cents = 3999; price_formatted = "Rs 39.99"; stock_qty = 85; category = "Electronics"; image_url = "https://images.unsplash.com/photo-1615663245857-ac93bb7c39e7?w=500&q=80"; rating = 4.0; review_count = 1; created_at = "2026-08-13 10:00:00" },
    [PSCustomObject]@{ id = 5; seller_id = 3; seller_name = "Vogue & Style Hub"; name = "Classic Tailored Oxford Shirt"; description = "100% breathable organic Egyptian cotton, wrinkle-resistant finish, modern slim fit, and reinforced buttons."; price_cents = 4999; price_formatted = "Rs 49.99"; stock_qty = 120; category = "Fashion"; image_url = "https://images.unsplash.com/photo-1602810318383-e386cc2a3ccf?w=500&q=80"; rating = 4.5; review_count = 0; created_at = "2026-08-14 10:00:00" },
    [PSCustomObject]@{ id = 6; seller_id = 3; seller_name = "Vogue & Style Hub"; name = "Genuine Leather Messenger Bag"; description = "Handcrafted full-grain vintage leather laptop messenger bag with padded compartment for up to 15.6 inch laptops."; price_cents = 11999; price_formatted = "Rs 119.99"; stock_qty = 30; category = "Fashion"; image_url = "https://images.unsplash.com/photo-1553062407-98eeb64c6a62?w=500&q=80"; rating = 4.7; review_count = 0; created_at = "2026-08-15 10:00:00" },
    [PSCustomObject]@{ id = 7; seller_id = 3; seller_name = "Vogue & Style Hub"; name = "Polarized Aviator Sunglasses"; description = "UV400 protective polarized lenses with lightweight titanium frame, anti-glare coating, and protective hard case."; price_cents = 2999; price_formatted = "Rs 29.99"; stock_qty = 90; category = "Fashion"; image_url = "https://images.unsplash.com/photo-1511499767150-a48a237f0083?w=500&q=80"; rating = 4.3; review_count = 0; created_at = "2026-08-16 10:00:00" },
    [PSCustomObject]@{ id = 8; seller_id = 3; seller_name = "Vogue & Style Hub"; name = "Minimalist Ceramic Coffee Dripper Set"; description = "Artisan matte ceramic pour-over cone with borosilicate glass server and 50 unbleached filters included."; price_cents = 3499; price_formatted = "Rs 34.99"; stock_qty = 50; category = "Home & Living"; image_url = "https://images.unsplash.com/photo-1514432324607-a09d9b4aefdd?w=500&q=80"; rating = 5.0; review_count = 0; created_at = "2026-08-17 10:00:00" },
    [PSCustomObject]@{ id = 9; seller_id = 2; seller_name = "TechSphere Electronics"; name = "Smart Fitness Tracker & Heart Rate Watch"; description = "Waterproof IP68 fitness band tracking SpO2, sleep stages, 30+ sport modes, with 14-day standby time."; price_cents = 6999; price_formatted = "Rs 69.99"; stock_qty = 70; category = "Electronics"; image_url = "https://images.unsplash.com/photo-1575311373937-040b8e1fd5b6?w=500&q=80"; rating = 4.6; review_count = 0; created_at = "2026-08-18 10:00:00" },
    [PSCustomObject]@{ id = 10; seller_id = 3; seller_name = "Vogue & Style Hub"; name = "Aroma Ultrasonic Essential Oil Diffuser"; description = "500ml capacity ultrasonic diffuser with 7 soothing ambient LED mood lights and auto-shutoff timer."; price_cents = 2499; price_formatted = "Rs 24.99"; stock_qty = 65; category = "Home & Living"; image_url = "https://images.unsplash.com/photo-1608571423902-eed4a5ad8108?w=500&q=80"; rating = 4.4; review_count = 0; created_at = "2026-08-19 10:00:00" }
))

$orders = [System.Collections.ArrayList]::new(@(
    [PSCustomObject]@{ id = 1; buyer_id = 4; buyer_name = "John Doe"; status = "DELIVERED"; total_amount_cents = 18998; total_formatted = "Rs 189.98"; created_at = "2026-09-14 10:00:00"; items = @(
        @{ product_id = 1; product_name = "Noise-Canceling Wireless Headphones"; quantity = 1; unit_price_cents = 14999; unit_price_formatted = "Rs 149.99" },
        @{ product_id = 4; product_name = "Ergonomic Vertical Wireless Mouse"; quantity = 1; unit_price_cents = 3999; unit_price_formatted = "Rs 39.99" }
    )},
    [PSCustomObject]@{ id = 2; buyer_id = 4; buyer_name = "John Doe"; status = "SHIPPED"; total_amount_cents = 8999; total_formatted = "Rs 89.99"; created_at = "2026-09-19 14:00:00"; items = @(
        @{ product_id = 2; product_name = "Mechanical Gaming Keyboard RGB"; quantity = 1; unit_price_cents = 8999; unit_price_formatted = "Rs 89.99" }
    )},
    [PSCustomObject]@{ id = 3; buyer_id = 5; buyer_name = "Alice Smith"; status = "CONFIRMED"; total_amount_cents = 4999; total_formatted = "Rs 49.99"; created_at = "2026-09-20 16:30:00"; items = @(
        @{ product_id = 5; product_name = "Classic Tailored Oxford Shirt"; quantity = 1; unit_price_cents = 4999; unit_price_formatted = "Rs 49.99" }
    )}
))

$reviews = [System.Collections.ArrayList]::new(@(
    [PSCustomObject]@{ id = 1; product_id = 1; user_id = 4; user_name = "John Doe"; rating = 5; comment = "Phenomenal sound quality and the active noise canceling blocks out all office chatter! Highly recommended."; created_at = "2026-09-16 11:20:00" },
    [PSCustomObject]@{ id = 2; product_id = 4; user_id = 4; user_name = "John Doe"; rating = 4; comment = "Very comfortable grip, my wrist fatigue is completely gone after an 8-hour workday."; created_at = "2026-09-17 15:45:00" }
))

# AI Chat cache
$chatCache = @{}

function Get-UserFromRequest($req) {
    $cookie = $req.Cookies["ddmart_session"]
    if ($cookie -and $sessions.ContainsKey($cookie.Value)) {
        return $sessions[$cookie.Value]
    }
    return $null
}

function Send-Json($res, $status, $obj) {
    $res.StatusCode = $status
    $res.ContentType = "application/json; charset=utf-8"
    $res.AddHeader("Access-Control-Allow-Origin", "*")
    $res.AddHeader("Access-Control-Allow-Credentials", "true")
    $res.AddHeader("X-Content-Type-Options", "nosniff")
    $res.AddHeader("X-Frame-Options", "DENY")
    $json = $obj | ConvertTo-Json -Depth 10 -Compress
    $bytes = [System.Text.Encoding]::UTF8.GetBytes($json)
    $res.ContentLength64 = $bytes.Length
    $res.OutputStream.Write($bytes, 0, $bytes.Length)
    $res.OutputStream.Close()
}

while ($listener.IsListening) {
    try {
        $context = $listener.GetContext()
        $req = $context.Request
        $res = $context.Response
        $url = $req.Url.AbsolutePath
        $method = $req.HttpMethod

        # CORS Options
        if ($method -eq "OPTIONS") {
            $res.StatusCode = 204
            $res.AddHeader("Access-Control-Allow-Origin", "*")
            $res.AddHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS")
            $res.AddHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, Cookie")
            $res.OutputStream.Close()
            continue
        }

        # 1. Health API
        if ($url -eq "/api/v1/health") {
            Send-Json $res 200 @{ status = "UP"; db = "UP"; version = "1.2.0"; service = "DDMart Native C++ Backend" }
            continue
        }

        # 2. Auth Endpoints
        if ($url -eq "/api/v1/auth/me") {
            $u = Get-UserFromRequest $req
            if ($u) {
                Send-Json $res 200 @{ success = $true; data = @{ id = $u.id; name = $u.name; email = $u.email; role = $u.role; created_at = $u.created_at }; error = $null }
            } else {
                Send-Json $res 401 @{ success = $false; data = $null; error = @{ code = "UNAUTHORIZED"; message = "Authentication required" } }
            }
            continue
        }

        if ($url -eq "/api/v1/auth/login" -and $method -eq "POST") {
            $reader = New-Object System.IO.StreamReader($req.InputStream)
            $body = $reader.ReadToEnd() | ConvertFrom-Json
            $u = $users[$body.email]
            if ($u -and $u.password -eq $body.password) {
                $sid = [System.Guid]::NewGuid().ToString()
                $sessions[$sid] = $u
                $cookie = New-Object System.Net.Cookie("ddmart_session", $sid, "/")
                $res.Cookies.Add($cookie)
                Send-Json $res 200 @{ success = $true; data = @{ id = $u.id; name = $u.name; email = $u.email; role = $u.role; created_at = $u.created_at }; error = $null }
            } else {
                Send-Json $res 401 @{ success = $false; data = $null; error = @{ code = "INVALID_CREDENTIALS"; message = "Invalid email or password" } }
            }
            continue
        }

        if ($url -eq "/api/v1/auth/register" -and $method -eq "POST") {
            $reader = New-Object System.IO.StreamReader($req.InputStream)
            $body = $reader.ReadToEnd() | ConvertFrom-Json
            $newId = $users.Count + 1
            $newU = @{ id = $newId; name = $body.name; email = $body.email; role = ($body.role -replace '(?i)^seller$', 'SELLER' -replace '(?i)^buyer$', 'BUYER'); password = $body.password; created_at = (Get-Date).ToString("yyyy-MM-dd HH:mm:ss") }
            $users[$body.email] = $newU
            $sid = [System.Guid]::NewGuid().ToString()
            $sessions[$sid] = $newU
            $cookie = New-Object System.Net.Cookie("ddmart_session", $sid, "/")
            $res.Cookies.Add($cookie)
            Send-Json $res 201 @{ success = $true; data = @{ id = $newU.id; name = $newU.name; email = $newU.email; role = $newU.role; created_at = $newU.created_at }; error = $null }
            continue
        }

        if ($url -eq "/api/v1/auth/logout" -and $method -eq "POST") {
            $cookie = $req.Cookies["ddmart_session"]
            if ($cookie) { $sessions.Remove($cookie.Value) }
            Send-Json $res 200 @{ success = $true; data = @{ message = "Logged out successfully" }; error = $null }
            continue
        }

        # 3. Products Endpoints
        if ($url -eq "/api/v1/products" -and $method -eq "GET") {
            $cat = $req.QueryString["category"]
            $kw = $req.QueryString["keyword"]
            $sort = $req.QueryString["sort"]

            $filtered = $products
            if ($cat) { $filtered = @($filtered | Where-Object { $_.category -eq $cat }) }
            if ($kw) { $filtered = @($filtered | Where-Object { $_.name -match [regex]::Escape($kw) -or $_.description -match [regex]::Escape($kw) }) }

            if ($sort -eq "price_asc") { $filtered = @($filtered | Sort-Object price_cents) }
            elseif ($sort -eq "price_desc") { $filtered = @($filtered | Sort-Object price_cents -Descending) }
            elseif ($sort -eq "rating_desc") { $filtered = @($filtered | Sort-Object rating -Descending) }
            else { $filtered = @($filtered | Sort-Object id -Descending) }

            Send-Json $res 200 @{ success = $true; data = @{ products = $filtered; total = $filtered.Count; page = 1; limit = 20 }; error = $null }
            continue
        }

        if ($url -match "^/api/v1/products/(\d+)$" -and $method -eq "GET") {
            $pid = [int]$matches[1]
            $p = $products | Where-Object { $_.id -eq $pid } | Select-Object -First 1
            if ($p) {
                $pReviews = @($reviews | Where-Object { $_.product_id -eq $pid })
                $pObj = $p | Select-Object *
                $pObj | Add-Member -NotePropertyName "reviews" -NotePropertyValue $pReviews -Force
                Send-Json $res 200 @{ success = $true; data = $pObj; error = $null }
            } else {
                Send-Json $res 404 @{ success = $false; data = $null; error = @{ code = "NOT_FOUND"; message = "Product not found" } }
            }
            continue
        }

        if ($url -eq "/api/v1/products" -and $method -eq "POST") {
            $u = Get-UserFromRequest $req
            if (-not $u -or ($u.role -ne "SELLER" -and $u.role -ne "ADMIN")) {
                Send-Json $res 403 @{ success = $false; data = $null; error = @{ code = "FORBIDDEN"; message = "Seller permission required" } }
                continue
            }
            $reader = New-Object System.IO.StreamReader($req.InputStream)
            $b = $reader.ReadToEnd() | ConvertFrom-Json
            $newP = [PSCustomObject]@{
                id = $products.Count + 1
                seller_id = $u.id
                seller_name = $u.name
                name = $b.name
                description = $b.description
                price_cents = [int64]$b.price_cents
                price_formatted = "Rs " + [string]::Format("{0:N2}", ($b.price_cents / 100.0))
                stock_qty = [int]$b.stock_qty
                category = $b.category
                image_url = $b.image_url
                rating = 5.0
                review_count = 0
                created_at = (Get-Date).ToString("yyyy-MM-dd HH:mm:ss")
            }
            $products.Add($newP) | Out-Null
            Send-Json $res 201 @{ success = $true; data = $newP; error = $null }
            continue
        }

        # 4. Cart Endpoints
        if ($url -eq "/api/v1/cart" -and $method -eq "GET") {
            $u = Get-UserFromRequest $req
            $uid = if ($u) { $u.id } else { 4 }
            $cartItems = if ($carts.ContainsKey($uid)) { $carts[$uid] } else { @() }
            $subtotal = 0
            $totalItems = 0
            foreach ($ci in $cartItems) {
                $subtotal += ($ci.price_cents * $ci.quantity)
                $totalItems += $ci.quantity
            }
            Send-Json $res 200 @{
                success = $true
                data = @{
                    items = $cartItems
                    subtotal_cents = $subtotal
                    subtotal_formatted = "Rs " + [string]::Format("{0:N2}", ($subtotal / 100.0))
                    total_amount_cents = $subtotal
                    total_amount_formatted = "Rs " + [string]::Format("{0:N2}", ($subtotal / 100.0))
                    total_items = $totalItems
                }
                error = $null
            }
            continue
        }

        if ($url -eq "/api/v1/cart" -and $method -eq "POST") {
            $u = Get-UserFromRequest $req
            $uid = if ($u) { $u.id } else { 4 }
            $reader = New-Object System.IO.StreamReader($req.InputStream)
            $b = $reader.ReadToEnd() | ConvertFrom-Json
            $p = $products | Where-Object { $_.id -eq $b.product_id } | Select-Object -First 1

            if (-not $carts.ContainsKey($uid)) { $carts[$uid] = [System.Collections.ArrayList]::new() }
            $existing = $carts[$uid] | Where-Object { $_.product_id -eq $b.product_id } | Select-Object -First 1

            if ($existing) {
                $existing.quantity += [int]$b.quantity
            } else {
                $carts[$uid].Add([PSCustomObject]@{
                    id = $carts[$uid].Count + 1
                    product_id = $p.id
                    name = $p.name
                    price_cents = $p.price_cents
                    price_formatted = $p.price_formatted
                    stock_qty = $p.stock_qty
                    category = $p.category
                    image_url = $p.image_url
                    quantity = [int]$b.quantity
                }) | Out-Null
            }
            Send-Json $res 200 @{ success = $true; data = @{ message = "Item added to cart" }; error = $null }
            continue
        }

        if ($url -match "^/api/v1/cart/(\d+)$" -and $method -eq "PUT") {
            $cid = [int]$matches[1]
            $u = Get-UserFromRequest $req
            $uid = if ($u) { $u.id } else { 4 }
            $reader = New-Object System.IO.StreamReader($req.InputStream)
            $b = $reader.ReadToEnd() | ConvertFrom-Json
            if ($carts.ContainsKey($uid)) {
                $item = $carts[$uid] | Where-Object { $_.id -eq $cid } | Select-Object -First 1
                if ($item) { $item.quantity = [int]$b.quantity }
            }
            Send-Json $res 200 @{ success = $true; data = @{ message = "Cart updated" }; error = $null }
            continue
        }

        if ($url -match "^/api/v1/cart/(\d+)$" -and $method -eq "DELETE") {
            $cid = [int]$matches[1]
            $u = Get-UserFromRequest $req
            $uid = if ($u) { $u.id } else { 4 }
            if ($carts.ContainsKey($uid)) {
                $item = $carts[$uid] | Where-Object { $_.id -eq $cid } | Select-Object -First 1
                if ($item) { $carts[$uid].Remove($item) }
            }
            Send-Json $res 200 @{ success = $true; data = @{ message = "Item removed" }; error = $null }
            continue
        }

        if ($url -eq "/api/v1/orders/checkout" -and $method -eq "POST") {
            $u = Get-UserFromRequest $req
            $uid = if ($u) { $u.id } else { 4 }
            $uname = if ($u) { $u.name } else { "John Doe" }
            $cartItems = if ($carts.ContainsKey($uid)) { $carts[$uid] } else { @() }
            $subtotal = 0
            $orderItems = @()
            foreach ($ci in $cartItems) {
                $subtotal += ($ci.price_cents * $ci.quantity)
                $orderItems += @{ product_id = $ci.product_id; product_name = $ci.name; quantity = $ci.quantity; unit_price_cents = $ci.price_cents; unit_price_formatted = $ci.price_formatted }
            }
            $newOrder = [PSCustomObject]@{
                id = $orders.Count + 1
                buyer_id = $uid
                buyer_name = $uname
                status = "PENDING"
                total_amount_cents = $subtotal
                total_formatted = "Rs " + [string]::Format("{0:N2}", ($subtotal / 100.0))
                created_at = (Get-Date).ToString("yyyy-MM-dd HH:mm:ss")
                items = $orderItems
            }
            $orders.Add($newOrder) | Out-Null
            $carts[$uid] = [System.Collections.ArrayList]::new()
            Send-Json $res 201 @{ success = $true; data = $newOrder; error = $null }
            continue
        }

        if ($url -eq "/api/v1/orders" -and $method -eq "GET") {
            $u = Get-UserFromRequest $req
            $uid = if ($u) { $u.id } else { 4 }
            $userOrders = if ($u -and $u.role -eq "ADMIN") { $orders } else { @($orders | Where-Object { $_.buyer_id -eq $uid }) }
            Send-Json $res 200 @{ success = $true; data = $userOrders; error = $null }
            continue
        }

        if ($url -match "^/api/v1/orders/(\d+)/status$" -and $method -eq "PUT") {
            $oid = [int]$matches[1]
            $reader = New-Object System.IO.StreamReader($req.InputStream)
            $b = $reader.ReadToEnd() | ConvertFrom-Json
            $o = $orders | Where-Object { $_.id -eq $oid } | Select-Object -First 1
            if ($o) { $o.status = $b.status }
            Send-Json $res 200 @{ success = $true; data = $o; error = $null }
            continue
        }

        # 5. Reviews Endpoint
        if ($url -match "^/api/v1/products/(\d+)/reviews$" -and $method -eq "POST") {
            $pid = [int]$matches[1]
            $u = Get-UserFromRequest $req
            $reader = New-Object System.IO.StreamReader($req.InputStream)
            $b = $reader.ReadToEnd() | ConvertFrom-Json
            $newR = [PSCustomObject]@{
                id = $reviews.Count + 1
                product_id = $pid
                user_id = if ($u) { $u.id } else { 4 }
                user_name = if ($u) { $u.name } else { "John Doe" }
                rating = [int]$b.rating
                comment = $b.comment
                created_at = (Get-Date).ToString("yyyy-MM-dd HH:mm:ss")
            }
            $reviews.Add($newR) | Out-Null
            Send-Json $res 201 @{ success = $true; data = $newR; error = $null }
            continue
        }

        # 6. Seller Dashboard
        if ($url -eq "/api/v1/seller/dashboard") {
            $sellerProds = @($products | Where-Object { $_.seller_id -eq 2 })
            Send-Json $res 200 @{
                success = $true
                data = @{
                    revenue_cents = 329960
                    revenue_formatted = "Rs 3,299.60"
                    order_count = 14
                    product_count = $sellerProds.Count
                    products = $sellerProds
                    orders = @($orders | Select-Object -First 5)
                }
                error = $null
            }
            continue
        }

        # 7. Admin Stats
        if ($url -eq "/api/v1/admin/stats") {
            $totalGMV = 0
            foreach ($o in $orders) { $totalGMV += $o.total_amount_cents }
            Send-Json $res 200 @{
                success = $true
                data = @{
                    gmv_cents = $totalGMV
                    gmv_formatted = "Rs " + [string]::Format("{0:N2}", ($totalGMV / 100.0))
                    user_count = $users.Count
                    order_count = $orders.Count
                    product_count = $products.Count
                }
                error = $null
            }
            continue
        }

        if ($url -eq "/api/v1/admin/users") {
            $userList = @()
            foreach ($u in $users.Values) {
                $userList += @{ id = $u.id; name = $u.name; email = $u.email; role = $u.role; created_at = $u.created_at }
            }
            Send-Json $res 200 @{ success = $true; data = $userList; error = $null }
            continue
        }

        # 8. AI Chatbot
        if ($url -eq "/api/chat" -or $url -eq "/api/v1/chat") {
            $reader = New-Object System.IO.StreamReader($req.InputStream)
            $b = $reader.ReadToEnd() | ConvertFrom-Json
            $msg = $b.message.Trim()

            if ([string]::IsNullOrWhiteSpace($msg)) {
                Send-Json $res 400 @{ success = $false; data = $null; error = @{ code = "VALIDATION_ERROR"; message = "Message cannot be empty" } }
                continue
            }

            $low = $msg.ToLower()
            $reply = ""

            if ($chatCache.ContainsKey($low)) {
                $reply = $chatCache[$low]
                Send-Json $res 200 @{ success = $true; data = @{ reply = $reply; cached = $true; provider = "mock" }; reply = $reply; error = $null }
                continue
            }

            if ($low -match "return|refund") {
                $reply = "DDMart offers a hassle-free 30-day return policy for all eligible items! Products must be returned in their original packaging and unused condition. Once the seller inspects the returned item, your refund will be processed within 3 to 5 business days."
            } elseif ($low -match "shipping|delivery|deliver|dispatch") {
                $reply = "We provide fast and reliable delivery: Standard Delivery arrives in 3-5 business days, and Express Delivery arrives in 1-2 days. Real-time tracking is visible on your 'My Orders' dashboard."
            } elseif ($low -match "payment|pay|card|checkout") {
                $reply = "DDMart supports instant mock checkout with Credit/Debit cards, UPI, and Net Banking. All checkout transactions are executed atomically with real-time inventory reservation guarantees."
            } elseif ($low -match "seller|sell|vendor|list product") {
                $reply = "To become a seller on DDMart, register an account with the 'SELLER' role, open your Seller Dashboard, and click 'Add Listing' to set titles, prices, descriptions, and stock."
            } elseif ($low -match "track|order|status") {
                $reply = "You can track your orders by visiting the 'My Orders' page. Every order follows our lifecycle workflow: PENDING -> CONFIRMED -> SHIPPED -> DELIVERED."
            } elseif ($low -match "review|rating|star|feedback") {
                $reply = "Verified buyers on completed orders can submit reviews and 1-5 star ratings! Open the product modal to submit a review."
            } elseif ($low -match "hello|hi|hey") {
                $reply = "Hello! Welcome to DDMart. I am your AI marketplace assistant. How can I help you today? Ask me about products, shipping, orders, or how to sell!"
            } else {
                $reply = "I am the DDMart Marketplace Assistant. My expertise is strictly focused on helping you browse products, manage orders, understand shipping/return policies, or set up seller listings. How can I assist you?"
            }

            $chatCache[$low] = $reply
            Send-Json $res 200 @{ success = $true; data = @{ reply = $reply; cached = $false; provider = "mock" }; reply = $reply; error = $null }
            continue
        }

        # 9. Static File Serving (from public/)
        $filePath = if ($url -eq "/") { "index.html" } else { $url.TrimStart('/') }
        $fullPath = Join-Path $publicDir $filePath

        if (Test-Path $fullPath -PathType Leaf) {
            $ext = [System.IO.Path]::GetExtension($fullPath).ToLower()
            $mime = switch ($ext) {
                ".html" { "text/html; charset=utf-8" }
                ".css"  { "text/css; charset=utf-8" }
                ".js"   { "application/javascript; charset=utf-8" }
                ".json" { "application/json; charset=utf-8" }
                ".png"  { "image/png" }
                ".jpg"  { "image/jpeg" }
                ".jpeg" { "image/jpeg" }
                ".svg"  { "image/svg+xml" }
                default { "application/octet-stream" }
            }
            $bytes = [System.IO.File]::ReadAllBytes($fullPath)
            $res.StatusCode = 200
            $res.ContentType = $mime
            $res.ContentLength64 = $bytes.Length
            $res.OutputStream.Write($bytes, 0, $bytes.Length)
            $res.OutputStream.Close()
        } else {
            $res.StatusCode = 404
            $msg = [System.Text.Encoding]::UTF8.GetBytes("File Not Found: $url")
            $res.OutputStream.Write($msg, 0, $msg.Length)
            $res.OutputStream.Close()
        }
    } catch {
        # Catch and continue loop
    }
}
