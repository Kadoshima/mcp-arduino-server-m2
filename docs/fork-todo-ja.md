# フォーク後の修正TODO（v1.0）

質問時刻：2026-01-15 21:12（JST）

## Phase 0：最初にやる（30分）
- [ ] フォークしてローカルにclone
- [ ] READMEを読んで起動確認（MCPサーバが立つこと）
- [ ] Windows環境で `arduino-cli` が叩けることを確認
- [ ] `MCP_SKETCH_DIR` を自分用に固定（例：`~/Documents/Arduino_MCP_Sketches/`）

## Phase 1：追加機能の設計を固める（0.5日）
- [ ] 追加するMCP Tool名を確定
  - `serial_write`
  - `pipeline_run_and_collect`
  - （任意）`serial_request_export`
- [ ] artifactsディレクトリ規約を確定
  - `artifacts/{run_id}/compile.log` など
- [ ] run_id生成規約を決める（例：`YYYYMMDD_HHMMSS_run###`）

## Phase 2：Serial “送信” 機能を追加（最重要 0.5〜1日）
> 既存が受信（monitor）中心なので、送信APIを必ず足す

- [ ] Tool追加：`serial_write(port, baud, data, append_newline)`
- [ ] 実装：指定COMに書き込み（タイムアウト付き）
- [ ] `control.log` に送信内容を追記
- [ ] 送信結果（bytes_written）を返す

✅ ここができると START/STOP/EXPORT をAIから打てる

## Phase 3：成果物管理（artifacts/run_id）を追加（0.5日）
- [ ] `artifacts/` をプロジェクト直下に追加
- [ ] `create_artifact_dir(run_id)` を実装
- [ ] `manifest.json` の雛形生成を実装
- [ ] compile/upload/serial monitorのログを run_id配下に保存するように変更
  - 既存がログファイル回転を持っていても、run単位保存に寄せる

## Phase 4：EXPORT（SD→Serial回収）を成立させる（1〜2日）
> v1.0のコア。SD抜き差し無しの自律回収

- [ ] ESP32側ファーム修正
  - [ ] `EXPORT run_id=...` 受信でCSV送信開始
  - [ ] `BEGIN/END` またはサイズ付き送信で完了判定可能にする
- [ ] PC側回収ロジック（どっちか採用）
  - [ ] 案A：serial monitorのログファイルから抽出（実装は楽だが汚い）
  - [ ] 案B：pyserialでEXPORT専用受信（堅い・おすすめ）
- [ ] MCP Tool追加（推奨）：`serial_export_sd_run(run_id, port, baud, dst_dir)`
  - [ ] `EXPORT`送信
  - [ ] 受信して `artifacts/{run_id}/sd/log.csv` に保存
  - [ ] 成功/失敗と再試行を返す

✅ ここまでで「回収」が自律化する

## Phase 5：analysis.run を追加（0.5〜1日）
- [ ] MCP Tool追加：`analysis_run(script_path, input_dir, output_dir, args)`
- [ ] 実装：Python subprocessで起動（timeout付き）
- [ ] stdout/stderrを `analysis.log` に保存
- [ ] 解析成果物を `artifacts/{run_id}/analysis/` に保存

## Phase 6：cloud.upload を追加（0.5〜1日）
- [ ] MCP Tool追加：`cloud_upload(src_dir, provider, bucket, prefix)`
- [ ] v1はS3互換（MinIO含む）でOK
- [ ] 成功したら `manifest.json` にアップロード先を追記

## Phase 7：pipeline.run_and_collect を追加（最終統合 1日）
> compile→upload→run→export→analysis→upload を1発実行にする

- [ ] MCP Tool追加：`pipeline_run_and_collect(config_json)`
- [ ] 実装ステップ
  - [ ] run_id生成 → artifacts作成
  - [ ] compile → upload
  - [ ] serial_write START
  - [ ] wait(duration_sec)
  - [ ] serial_write STOP
  - [ ] （任意）serial_monitor_start + read + stop（ログ採取）
  - [ ] serial_export_sd_run（回収）
  - [ ] analysis_run（解析）
  - [ ] cloud_upload（送信）
  - [ ] manifest確定
- [ ] 失敗時の復旧設計
  - [ ] exportだけリトライ可能にする
  - [ ] `next_action_hint` を返す

## Phase 8：v1.0の仕上げ（0.5〜1日）
- [ ] WindowsでのCOM自動検出（任意）
- [ ] “危険コマンド禁止”のガード（許可リスト）
- [ ] READMEにあなた用の実行例を追記
- [ ] 最小のE2Eテスト（実機で1ラン完走）

## 最短の実装順（結論）
迷ったらこの順番が最短で価値が出る：

1. serial_write
2. artifacts/run_id保存
3. EXPORT回収（serial_export_sd_run）
4. pipeline_run_and_collect
5. analysis → cloud

## 工数目安（自前コーディング）
- 素：4〜6日
- バッファ込み：6〜9日
