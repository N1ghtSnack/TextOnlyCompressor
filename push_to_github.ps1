# TextOnlyCompressor - GitHub 推送脚本
# 用途: 一键将项目推送到GitHub

Write-Host "`n╔════════════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║     TextOnlyCompressor - GitHub 仓库推送工具                 ║" -ForegroundColor Cyan
Write-Host "╚════════════════════════════════════════════════════════════════╝`n" -ForegroundColor Cyan

# 获取用户信息
Write-Host "请输入以下信息来设置远程仓库:`n" -ForegroundColor Yellow

$github_username = Read-Host "输入您的 GitHub 用户名"
$github_token = Read-Host "输入您的 GitHub Token 或个人访问令牌 (PAT)" -AsSecureString

if ([string]::IsNullOrWhiteSpace($github_username)) {
    Write-Host "✗ 用户名不能为空，已取消操作" -ForegroundColor Red
    exit 1
}

# 转换 Token
$bstr = [System.Runtime.InteropServices.Marshal]::SecureStringToBSTR($github_token)
$token_plain = [System.Runtime.InteropServices.Marshal]::PtrToStringAuto($bstr)

if ([string]::IsNullOrWhiteSpace($token_plain)) {
    Write-Host "✗ Token 不能为空，已取消操作" -ForegroundColor Red
    exit 1
}

# 构建仓库URL
$repo_url = "https://${github_username}:${token_plain}@github.com/${github_username}/TextOnlyCompressor.git"
$repo_url_display = "https://github.com/${github_username}/TextOnlyCompressor.git"

Write-Host "`n[操作步骤]" -ForegroundColor Green

# 检查远程仓库是否已存在
Write-Host "1. 检查远程仓库配置..."
$remote = git remote get-url origin 2>$null
if ($remote) {
    Write-Host "   ✓ 远程仓库已配置: $remote"
} else {
    Write-Host "   ✗ 远程仓库未配置，正在添加..."
    git remote add origin $repo_url
    Write-Host "   ✓ 远程仓库已添加"
}

# 切换主分支名称
Write-Host "2. 切换到主分支..."
git branch -M main
Write-Host "   ✓ 已切换到 main 分支"

# 推送代码和标签
Write-Host "3. 推送代码到 GitHub..."
git push -u origin main --tags

if ($LASTEXITCODE -eq 0) {
    Write-Host "`n╔════════════════════════════════════════════════════════════════╗" -ForegroundColor Green
    Write-Host "║                    推送成功！                                  ║" -ForegroundColor Green
    Write-Host "╚════════════════════════════════════════════════════════════════╝" -ForegroundColor Green
    
    Write-Host "`n[仓库信息]" -ForegroundColor Yellow
    Write-Host "  仓库名称: TextOnlyCompressor"
    Write-Host "  仓库链接: $repo_url_display"
    Write-Host "  版本标签: v0.1"
    Write-Host "`n[查看仓库]" -ForegroundColor Yellow
    Write-Host "  • 在浏览器中打开: $repo_url_display"
    Write-Host "  • 或访问: https://github.com/${github_username}/TextOnlyCompressor/releases/tag/v0.1`n"
} else {
    Write-Host "`n✗ 推送失败！请检查以下内容:" -ForegroundColor Red
    Write-Host "  1. GitHub 用户名和 Token 是否正确"
    Write-Host "  2. 是否已在 GitHub 上创建 TextOnlyCompressor 仓库"
    Write-Host "  3. 网络连接是否正常"
}

# 清除敏感信息
[System.Runtime.InteropServices.Marshal]::ZeroFreeBSTR($bstr)
