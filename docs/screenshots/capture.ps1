param(
    [string]$Action = "capture",
    [int]$W = 1440,
    [int]$H = 900,
    [float]$X = 0,
    [float]$Y = 0,
    [int]$Key = 0,
    [int]$Sleep = 400,
    [string]$Path = "shot.png"
)

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Text;
using System.Runtime.InteropServices;
public class W {
    public delegate bool EnumProc(IntPtr h, IntPtr l);
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr l);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int cmd);
    [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr after, int x, int y, int cx, int cy, uint flags);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")] public static extern void mouse_event(uint f, uint dx, uint dy, uint d, UIntPtr e);
    [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
    public struct RECT { public int Left, Top, Right, Bottom; }
    public struct POINT { public int X, Y; }
}
"@

[W]::SetProcessDPIAware() | Out-Null

function Find-Game {
    $script:found = [IntPtr]::Zero
    [W]::EnumWindows({ param($h, $l)
        $sb = New-Object System.Text.StringBuilder 256
        [W]::GetWindowText($h, $sb, 256) | Out-Null
        if ($sb.ToString() -eq "DoomsGame") { $script:found = $h; return $false }
        return $true
    }, [IntPtr]::Zero) | Out-Null
    return $script:found
}

if ($Action -eq "launch") {
    $exe = (Resolve-Path ".\x64\Debug\Template.exe").Path
    $p = Start-Process -FilePath $exe -WorkingDirectory (Get-Location) -PassThru
    $hwnd = [IntPtr]::Zero
    for ($i = 0; $i -lt 2000; $i++) {
        $hwnd = Find-Game
        if ($hwnd -ne [IntPtr]::Zero) { break }
        Start-Sleep -Milliseconds 2
    }
    if ($hwnd -eq [IntPtr]::Zero) { Write-Error "Game window not found"; exit 1 }
    # Move to secondary monitor immediately (x=-1920..0) where GL windows persist
    $cr = New-Object W+RECT; [W]::GetClientRect($hwnd, [ref]$cr) | Out-Null
    $wr = New-Object W+RECT; [W]::GetWindowRect($hwnd, [ref]$wr) | Out-Null
    $ow = ($wr.Right - $wr.Left) - $cr.Right + $W
    $oh = ($wr.Bottom - $wr.Top) - $cr.Bottom + $H
    [W]::SetWindowPos($hwnd, [IntPtr]::Zero, -1920, 0, $ow, $oh, 0x0040) | Out-Null
    Start-Sleep -Milliseconds 500
    $cr2 = New-Object W+RECT; [W]::GetClientRect($hwnd, [ref]$cr2) | Out-Null
    Write-Output ("launched pid=" + $p.Id + " hwnd=" + $hwnd + " client=" + $cr2.Right + "x" + $cr2.Bottom + " exited=" + $p.HasExited)
    exit 0
}

$hwnd = Find-Game
if ($hwnd -eq [IntPtr]::Zero) { Write-Error "Game window not found"; exit 1 }
[W]::ShowWindow($hwnd, 9) | Out-Null
[W]::SetForegroundWindow($hwnd) | Out-Null
Start-Sleep -Milliseconds 250

function Get-ClientSize() {
    $r = New-Object W+RECT
    [W]::GetClientRect($hwnd, [ref]$r) | Out-Null
    return @(($r.Right - $r.Left), ($r.Bottom - $r.Top))
}
function Get-ClientOrigin() {
    $p = New-Object W+POINT
    $p.X = 0; $p.Y = 0
    [W]::ClientToScreen($hwnd, [ref]$p) | Out-Null
    return @($p.X, $p.Y)
}

switch ($Action) {
    "click" {
        $s = Get-ClientSize; $cw = $s[0]; $ch = $s[1]
        $scale = [Math]::Min($cw / 1280.0, $ch / 720.0)
        $offX = ($cw - 1280.0 * $scale) / 2.0
        $offY = ($ch - 720.0 * $scale) / 2.0
        $cx = [int][Math]::Round($offX + $X * $scale)
        $cy = [int][Math]::Round($offY + (720.0 - $Y) * $scale)
        $o = Get-ClientOrigin
        [W]::SetCursorPos($o[0] + $cx, $o[1] + $cy) | Out-Null
        Start-Sleep -Milliseconds 200
        [W]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)
        Start-Sleep -Milliseconds 80
        [W]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)
        Write-Output ("clicked world=" + $X + "," + $Y + " client=" + $cx + "," + $cy + " size=" + $cw + "x" + $ch + " origin=" + $o[0] + "," + $o[1])
    }
    "move" {
        $s = Get-ClientSize; $cw = $s[0]; $ch = $s[1]
        $scale = [Math]::Min($cw / 1280.0, $ch / 720.0)
        $offX = ($cw - 1280.0 * $scale) / 2.0
        $offY = ($ch - 720.0 * $scale) / 2.0
        $cx = [int][Math]::Round($offX + $X * $scale)
        $cy = [int][Math]::Round($offY + (720.0 - $Y) * $scale)
        $o = Get-ClientOrigin
        [W]::SetCursorPos($o[0] + $cx, $o[1] + $cy) | Out-Null
        Start-Sleep -Milliseconds 400
        Write-Output ("moved world=" + $X + "," + $Y)
    }
    "key" {
        [W]::keybd_event([byte]$Key, 0, 0, [UIntPtr]::Zero)
        Start-Sleep -Milliseconds 80
        [W]::keybd_event([byte]$Key, 0, 2, [UIntPtr]::Zero)
        Write-Output ("key=" + $Key)
    }
    "capture" {
        $s = Get-ClientSize; $o = Get-ClientOrigin
        $bmp = New-Object System.Drawing.Bitmap($s[0], $s[1])
        $g = [System.Drawing.Graphics]::FromImage($bmp)
        $g.CopyFromScreen($o[0], $o[1], 0, 0, (New-Object System.Drawing.Size($s[0], $s[1])), [System.Drawing.CopyPixelOperation]::SourceCopy)
        $bmp.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
        $g.Dispose(); $bmp.Dispose()
        Write-Output ("saved " + $Path + " " + $s[0] + "x" + $s[1] + " at " + $o[0] + "," + $o[1])
    }
}
Start-Sleep -Milliseconds $Sleep
