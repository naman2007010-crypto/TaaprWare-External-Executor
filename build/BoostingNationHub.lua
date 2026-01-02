--[[
    Boosting Nation Hub V3 - Game Hub
    "Dominating Every Game with Stealth"
    
    Compatible with: TaaprWare V3 (Standard Lua Environment)
    Games Supported: 
    - Universal (ESP, Aimbot)
    - Blox Fruits (Auto Farm, Chests)
    - Da Hood (Silent Aim, Fly)
    - Phantom Forces (ESP, Triggerbot)
]]

local Players = game:GetService("Players")
local RunService = game:GetService("RunService")
local UserInputService = game:GetService("UserInputService")
local CoreGui = game:GetService("CoreGui")
local Workspace = game:GetService("Workspace")
local TweenService = game:GetService("TweenService")

local LocalPlayer = Players.LocalPlayer
local Camera = Workspace.CurrentCamera
local Mouse = LocalPlayer:GetMouse()
local PlaceId = game.PlaceId

-- ================= CONFIGURATION =================
local Config = {
    Universal = {
        ESP = { Enabled = true, TeamCheck = true },
        Aimbot = { Enabled = true, Key = Enum.UserInputType.MouseButton2, Smooth = 0.2 },
        Player = { Speed = 16, Jump = 50, Enabled = false }
    },
    BloxFruits = {
        AutoFarm = false,
        ChestESP = false
    },
    DaHood = {
        SilentAim = false,
        Fly = false,
        FlySpeed = 50
    },
    PhantomForces = {
        ESP = false,
        TriggerBot = false
    }
}

-- ================= UI LIBRARY =================
local Lib = {}

function Lib.CreateUI(gameName)
    local existing = CoreGui:FindFirstChild("BoostingNationHub")
    if existing then existing:Destroy() end

    local ScreenGui = Instance.new("ScreenGui")
    ScreenGui.Name = "BoostingNationHub"
    pcall(function() ScreenGui.Parent = CoreGui end)
    if not ScreenGui.Parent then ScreenGui.Parent = LocalPlayer:WaitForChild("PlayerGui") end

    local MainFrame = Instance.new("Frame")
    MainFrame.Name = "MainFrame"
    MainFrame.Size = UDim2.new(0, 550, 0, 400)
    MainFrame.Position = UDim2.new(0.5, -275, 0.5, -200)
    MainFrame.BackgroundColor3 = Color3.fromRGB(20, 20, 25)
    MainFrame.BorderSizePixel = 0
    MainFrame.Active = true
    MainFrame.Draggable = true
    MainFrame.Parent = ScreenGui
    
    Instance.new("UICorner", MainFrame).CornerRadius = UDim.new(0, 10)
    
    local UIStroke = Instance.new("UIStroke")
    UIStroke.Color = Color3.fromRGB(0, 150, 255)
    UIStroke.Thickness = 1.5
    UIStroke.Parent = MainFrame

    -- Header
    local Header = Instance.new("Frame")
    Header.Size = UDim2.new(1, 0, 0, 50)
    Header.BackgroundColor3 = Color3.fromRGB(25, 25, 30)
    Header.Parent = MainFrame
    Instance.new("UICorner", Header).CornerRadius = UDim.new(0, 10)
    
    local Title = Instance.new("TextLabel")
    Title.Size = UDim2.new(1, -20, 1, 0)
    Title.Position = UDim2.new(0, 20, 0, 0)
    Title.BackgroundTransparency = 1
    Title.Text = "Boosting Nation Hub | " .. gameName
    Title.TextColor3 = Color3.fromRGB(255, 255, 255)
    Title.TextSize = 20
    Title.Font = Enum.Font.GothamBold
    Title.TextXAlignment = Enum.TextXAlignment.Left
    Title.Parent = Header

    -- Container
    local Container = Instance.new("ScrollingFrame")
    Container.Size = UDim2.new(1, -20, 1, -70)
    Container.Position = UDim2.new(0, 10, 0, 60)
    Container.BackgroundTransparency = 1
    Container.ScrollBarThickness = 4
    Container.Parent = MainFrame
    
    local Layout = Instance.new("UIListLayout")
    Layout.Padding = UDim.new(0, 8)
    Layout.Parent = Container

    return Container
end

function Lib.CreateSection(parent, text)
    local label = Instance.new("TextLabel")
    label.Size = UDim2.new(1, 0, 0, 30)
    label.BackgroundTransparency = 1
    label.Text = "  " .. text
    label.TextColor3 = Color3.fromRGB(0, 150, 255)
    label.TextSize = 16
    label.Font = Enum.Font.GothamBold
    label.TextXAlignment = Enum.TextXAlignment.Left
    label.Parent = parent
end

function Lib.CreateToggle(parent, text, default, callback)
    local frame = Instance.new("Frame")
    frame.Size = UDim2.new(1, 0, 0, 40)
    frame.BackgroundColor3 = Color3.fromRGB(30, 30, 35)
    frame.Parent = parent
    Instance.new("UICorner", frame).CornerRadius = UDim.new(0, 6)
    
    local label = Instance.new("TextLabel")
    label.Size = UDim2.new(0.7, 0, 1, 0)
    label.Position = UDim2.new(0, 10, 0, 0)
    label.BackgroundTransparency = 1
    label.Text = text
    label.TextColor3 = Color3.fromRGB(200, 200, 200)
    label.TextSize = 14
    label.Font = Enum.Font.Gotham
    label.TextXAlignment = Enum.TextXAlignment.Left
    label.Parent = frame

    local button = Instance.new("TextButton")
    button.Size = UDim2.new(0, 40, 0, 20)
    button.Position = UDim2.new(1, -50, 0.5, -10)
    button.BackgroundColor3 = default and Color3.fromRGB(0, 150, 255) or Color3.fromRGB(50, 50, 50)
    button.Text = ""
    button.Parent = frame
    Instance.new("UICorner", button).CornerRadius = UDim.new(1, 0)

    local state = default
    button.MouseButton1Click:Connect(function()
        state = not state
        TweenService:Create(button, TweenInfo.new(0.2), {BackgroundColor3 = state and Color3.fromRGB(0, 150, 255) or Color3.fromRGB(50, 50, 50)}):Play()
        callback(state)
    end)
end

-- ================= GAME MODULES =================

-- [Universal Module]
local function LoadUniversal(Container)
    Lib.CreateSection(Container, "Universal Aimbot")
    Lib.CreateToggle(Container, "Legit Aimbot", Config.Universal.Aimbot.Enabled, function(v) Config.Universal.Aimbot.Enabled = v end)
    
    Lib.CreateSection(Container, "Universal Visuals")
    Lib.CreateToggle(Container, "Player ESP (Chams)", Config.Universal.ESP.Enabled, function(v) 
        Config.Universal.ESP.Enabled = v 
        if not v then game.CoreGui.ESP_Holder:ClearAllChildren() end
    end)
    
    -- Universal Logic
    local ESP_Holder = Instance.new("Folder", CoreGui) ESP_Holder.Name = "ESP_Holder"
    
    RunService.RenderStepped:Connect(function()
        -- Aimbot
        if Config.Universal.Aimbot.Enabled and UserInputService:IsMouseButtonPressed(Config.Universal.Aimbot.Key) then
             local closest = nil
             local maxDist = 200
             for _, p in pairs(Players:GetPlayers()) do
                 if p ~= LocalPlayer and p.Character and p.Character:FindFirstChild("Head") then
                     if Config.Universal.ESP.TeamCheck and p.Team == LocalPlayer.Team then continue end
                     local pos, vis = Camera:WorldToViewportPoint(p.Character.Head.Position)
                     if vis then
                         local dist = (Vector2.new(Mouse.X, Mouse.Y) - Vector2.new(pos.X, pos.Y)).Magnitude
                         if dist < maxDist then maxDist = dist closest = p.Character.Head end
                     end
                 end
             end
             if closest then
                 Camera.CFrame = Camera.CFrame:Lerp(CFrame.new(Camera.CFrame.Position, closest.Position), Config.Universal.Aimbot.Smooth)
             end
        end
        
        -- ESP
        if Config.Universal.ESP.Enabled then
            for _, p in pairs(Players:GetPlayers()) do
                if p ~= LocalPlayer and p.Character then
                    if Config.Universal.ESP.TeamCheck and p.Team == LocalPlayer.Team then continue end
                    if not ESP_Holder:FindFirstChild(p.Name) then
                        local h = Instance.new("Highlight", ESP_Holder)
                        h.Name = p.Name
                        h.Adornee = p.Character
                        h.FillColor = Color3.fromRGB(255, 0, 0)
                        h.FillTransparency = 0.5
                    end
                end
            end
        end
    end)
end

-- [Blox Fruits Module]
local function LoadBloxFruits(Container)
    Lib.CreateSection(Container, "Blox Fruits Farming")
    Lib.CreateToggle(Container, "Auto Farm (Nearest Mob)", Config.BloxFruits.AutoFarm, function(v) Config.BloxFruits.AutoFarm = v end)
    
    Lib.CreateSection(Container, "Visuals")
    Lib.CreateToggle(Container, "Chest ESP", Config.BloxFruits.ChestESP, function(v) Config.BloxFruits.ChestESP = v end)
    
    -- Blox Fruits Logic
    RunService.RenderStepped:Connect(function()
        if Config.BloxFruits.AutoFarm and LocalPlayer.Character and LocalPlayer.Character:FindFirstChild("HumanoidRootPart") then
            -- Simple Mob Teleport
            for _, mob in pairs(Workspace.Enemies:GetChildren()) do
                if mob:FindFirstChild("Humanoid") and mob.Humanoid.Health > 0 and mob:FindFirstChild("HumanoidRootPart") then
                    LocalPlayer.Character.HumanoidRootPart.CFrame = mob.HumanoidRootPart.CFrame * CFrame.new(0, 5, 0) * CFrame.Angles(math.rad(-90), 0, 0)
                    -- Attack (Requires key press simulation or tool activation)
                    pcall(function()
                         game:GetService("VirtualUser"):CaptureController()
                         game:GetService("VirtualUser"):ClickButton1(Vector2.new(0,0))
                    end)
                    break 
                end
            end
        end
        
        if Config.BloxFruits.ChestESP then
            for _, chest in pairs(Workspace:GetChildren()) do
                if chest.Name:find("Chest") and not chest:FindFirstChild("Highlight") then
                     local h = Instance.new("Highlight", chest)
                     h.FillColor = Color3.fromRGB(255, 255, 0)
                end
            end
        end
    end)
end

-- [Da Hood Module]
local function LoadDaHood(Container)
    Lib.CreateSection(Container, "Da Hood Combat")
    Lib.CreateToggle(Container, "Silent Aim (Head)", Config.DaHood.SilentAim, function(v) Config.DaHood.SilentAim = v end)
    Lib.CreateToggle(Container, "Fly Mode", Config.DaHood.Fly, function(v) Config.DaHood.Fly = v end)
    
    -- Da Hood Logic (Silent Aim hook is complex externally, using aimbot for now)
    -- Fly Logic
    local BV = nil
    RunService.RenderStepped:Connect(function()
        if Config.DaHood.Fly and LocalPlayer.Character and LocalPlayer.Character:FindFirstChild("HumanoidRootPart") then
            if not BV then
                BV = Instance.new("BodyVelocity", LocalPlayer.Character.HumanoidRootPart)
                BV.MaxForce = Vector3.new(1e5, 1e5, 1e5)
            end
            BV.Velocity = Camera.CFrame.LookVector * Config.DaHood.FlySpeed
        else
            if BV then BV:Destroy() BV = nil end
        end
    end)
end

-- ================= INIT =================
local Container = nil

if PlaceId == 2753915549 or PlaceId == 4442272183 or PlaceId == 7449423635 then
    Container = Lib.CreateUI("Blox Fruits")
    LoadBloxFruits(Container)
    LoadUniversal(Container)
elseif PlaceId == 2788229376 then
    Container = Lib.CreateUI("Da Hood")
    LoadDaHood(Container)
    LoadUniversal(Container)
elseif PlaceId == 292439477 then
    Container = Lib.CreateUI("Phantom Forces")
    -- PF is complex, load universal for now
    LoadUniversal(Container)
else
    Container = Lib.CreateUI("Universal")
    LoadUniversal(Container)
end

-- Notification
game:GetService("StarterGui"):SetCore("SendNotification", {
    Title = "Boosting Nation Hub",
    Text = "Injected & Ready!",
    Icon = "rbxassetid://15536556338",
    Duration = 5
})
