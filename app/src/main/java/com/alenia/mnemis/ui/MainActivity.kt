package com.alenia.mnemis.ui

import android.Manifest
import android.content.ContentUris
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.provider.MediaStore
import android.provider.Settings
import android.view.LayoutInflater
import android.view.View
import android.widget.FrameLayout
import android.widget.ProgressBar
import android.widget.Toast
import androidx.activity.OnBackPressedCallback
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import androidx.core.content.FileProvider
import androidx.core.view.GravityCompat
import androidx.core.widget.doAfterTextChanged
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.GridLayoutManager
import androidx.recyclerview.widget.LinearLayoutManager
import com.alenia.mnemis.BuildConfig
import com.alenia.mnemis.R
import com.alenia.mnemis.databinding.ActivityMainBinding
import com.alenia.mnemis.databinding.DialogOpenUrlBinding
import com.alenia.mnemis.model.PluginItem
import com.alenia.mnemis.model.VideoAlbum
import com.alenia.mnemis.model.VideoItem
import com.alenia.mnemis.plugin.DynamicPluginManager
import com.alenia.mnemis.plugin.MnemisPluginRegistry
import com.alenia.mnemis.prefs.MnemisPreferences
import com.alenia.mnemis.update.MnemisUpdater
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File

class MainActivity : AppCompatActivity() {

    private enum class Section {
        HOME, VIDEOS, WEB, PLUGINS, DOWNLOADS, SETTINGS
    }

    private lateinit var binding: ActivityMainBinding
    private lateinit var prefs: MnemisPreferences

    private var currentSection = Section.HOME
    private var allVideos: List<VideoItem> = emptyList()
    private var selectedAlbumName: String? = null
    private var currentQuery: String = ""

    private val albumAdapter = AlbumAdapter { album ->
        selectedAlbumName = album.name
        applySearchFilter()
    }

    private val videoAdapter = VideoAdapter { videoItem ->
        openPlayer(videoItem.uri.toString(), videoItem.title)
    }

    private val downloadsAdapter = VideoAdapter { videoItem ->
        openPlayer(videoItem.uri.toString(), videoItem.title)
    }

    private lateinit var pluginAdapter: PluginAdapter

    private val permissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { isGranted ->
        if (isGranted) {
            loadLocalVideos()
        } else {
            showPermissionDeniedState()
        }
    }

    private var pendingInstallApk: File? = null
    private var updateDownloadJob: Job? = null

    private val installPermissionLauncher = registerForActivityResult(
        ActivityResultContracts.StartActivityForResult()
    ) {
        val pendingApk = pendingInstallApk
        pendingInstallApk = null
        if (pendingApk != null && pendingApk.exists()) {
            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O ||
                packageManager.canRequestPackageInstalls()
            ) {
                startApkInstall(pendingApk)
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        prefs = MnemisPreferences(this)
        DynamicPluginManager.loadAll(this)

        setupDrawerAndToolbar()
        setupHomeSection()
        setupVideosSection()
        setupWebSection()
        setupPluginsSection()
        setupDownloadsSection()
        setupSettingsSection()
        setupBackNavigation()

        switchSection(Section.HOME)
        handleIncomingIntent(intent)
        checkPermissionAndLoadVideos()
        checkAppUpdates(manual = false)
    }

    override fun onResume() {
        super.onResume()
        refreshHomeCard()
    }

    private fun setupDrawerAndToolbar() {
        binding.btnMenu.setOnClickListener {
            binding.drawerLayout.openDrawer(GravityCompat.START)
        }

        binding.navigationView.setCheckedItem(R.id.nav_home)
        binding.navigationView.setNavigationItemSelectedListener { item ->
            when (item.itemId) {
                R.id.nav_home -> switchSection(Section.HOME)
                R.id.nav_videos -> switchSection(Section.VIDEOS)
                R.id.nav_web -> switchSection(Section.WEB)
                R.id.nav_plugins -> switchSection(Section.PLUGINS)
                R.id.nav_downloads -> switchSection(Section.DOWNLOADS)
                R.id.nav_settings -> switchSection(Section.SETTINGS)
            }
            binding.drawerLayout.closeDrawer(GravityCompat.START)
            true
        }

        binding.btnSearchToggle.setOnClickListener {
            val isVisible = binding.etSearchBar.visibility == View.VISIBLE
            if (isVisible) {
                binding.etSearchBar.setText("")
                binding.etSearchBar.visibility = View.GONE
                binding.tvToolbarTitle.visibility = View.VISIBLE
            } else {
                binding.tvToolbarTitle.visibility = View.GONE
                binding.etSearchBar.visibility = View.VISIBLE
                binding.etSearchBar.requestFocus()
            }
        }

        binding.etSearchBar.doAfterTextChanged { editable ->
            currentQuery = editable?.toString()?.trim().orEmpty()
            applySearchFilter()
        }

        binding.fabOpenLink.setOnClickListener {
            showOpenUrlDialog()
        }
    }

    private fun switchSection(section: Section) {
        currentSection = section
        binding.sectionHome.visibility = if (section == Section.HOME) View.VISIBLE else View.GONE
        binding.sectionVideos.visibility = if (section == Section.VIDEOS) View.VISIBLE else View.GONE
        binding.sectionWeb.visibility = if (section == Section.WEB) View.VISIBLE else View.GONE
        binding.sectionPlugins.visibility = if (section == Section.PLUGINS) View.VISIBLE else View.GONE
        binding.sectionDownloads.visibility = if (section == Section.DOWNLOADS) View.VISIBLE else View.GONE
        binding.sectionSettings.visibility = if (section == Section.SETTINGS) View.VISIBLE else View.GONE

        val showFab = section == Section.HOME || section == Section.VIDEOS || section == Section.DOWNLOADS
        binding.fabOpenLink.visibility = if (showFab) View.VISIBLE else View.GONE

        val showSearch = section == Section.VIDEOS || section == Section.PLUGINS || section == Section.DOWNLOADS
        binding.btnSearchToggle.visibility = if (showSearch) View.VISIBLE else View.GONE
        if (!showSearch) {
            binding.etSearchBar.setText("")
            binding.etSearchBar.visibility = View.GONE
            binding.tvToolbarTitle.visibility = View.VISIBLE
        }

        binding.tvToolbarTitle.text = when (section) {
            Section.HOME -> getString(R.string.nav_home)
            Section.VIDEOS -> getString(R.string.nav_videos)
            Section.WEB -> getString(R.string.nav_web)
            Section.PLUGINS -> getString(R.string.nav_plugins)
            Section.DOWNLOADS -> getString(R.string.nav_downloads)
            Section.SETTINGS -> getString(R.string.nav_settings)
        }

        if (section == Section.HOME) {
            refreshHomeCard()
        } else if (section == Section.SETTINGS) {
            loadSettingsIntoUi()
        } else if (section == Section.PLUGINS) {
            refreshPluginsList()
        }
    }

    private fun setupHomeSection() {
        binding.btnHomeOpenWeb.setOnClickListener {
            binding.navigationView.setCheckedItem(R.id.nav_web)
            switchSection(Section.WEB)
        }
        binding.btnHomeMyVideos.setOnClickListener {
            binding.navigationView.setCheckedItem(R.id.nav_videos)
            switchSection(Section.VIDEOS)
        }
        binding.btnHomePlugins.setOnClickListener {
            binding.navigationView.setCheckedItem(R.id.nav_plugins)
            switchSection(Section.PLUGINS)
        }
        refreshHomeCard()
    }

    private fun refreshHomeCard() {
        val last = prefs.getLastPlayed()
        if (last == null) {
            binding.cardLastPlayed.visibility = View.GONE
            binding.tvHomeEmpty.visibility = View.VISIBLE
            return
        }
        binding.tvHomeEmpty.visibility = View.GONE
        binding.cardLastPlayed.visibility = View.VISIBLE
        binding.tvLastPlayedTitle.text = last.title
        binding.tvLastPlayedType.text = if (last.isWeb) {
            getString(R.string.home_type_web)
        } else {
            getString(R.string.home_type_local)
        }
        binding.btnResumeLastPlayed.setOnClickListener {
            if (last.isWeb) {
                openWebPlayer(last.source, last.title)
            } else {
                openPlayer(last.source, last.title)
            }
        }
    }

    private fun setupVideosSection() {
        binding.recyclerViewAlbums.apply {
            layoutManager = GridLayoutManager(this@MainActivity, 2)
            adapter = albumAdapter
        }

        binding.recyclerViewVideos.apply {
            layoutManager = LinearLayoutManager(this@MainActivity)
            adapter = videoAdapter
        }

        binding.btnBackToAlbums.setOnClickListener {
            selectedAlbumName = null
            applySearchFilter()
        }

        binding.sectionVideos.setOnRefreshListener {
            loadLocalVideos()
        }

        binding.btnGrantPermission.setOnClickListener {
            requestStoragePermission()
        }
    }

    private fun setupWebSection() {
        binding.btnLaunchWebUrl.setOnClickListener {
            val rawUrl = binding.etWebSectionUrl.text?.toString()?.trim().orEmpty()
            if (rawUrl.isEmpty()) {
                Toast.makeText(this, "Ingresa una URL válida", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }
            val normalized = if (rawUrl.startsWith("http://") || rawUrl.startsWith("https://")) {
                rawUrl
            } else {
                "https://$rawUrl"
            }
            openWebPlayer(normalized, "Web Player")
        }

        binding.btnQuickYouTube.setOnClickListener {
            openWebPlayer("https://m.youtube.com", "YouTube")
        }

        binding.btnQuickTwitch.setOnClickListener {
            openWebPlayer("https://m.twitch.tv", "Twitch")
        }
    }

    private fun setupPluginsSection() {
        pluginAdapter = PluginAdapter(
            onToggleEnabled = { plugin, enabled ->
                prefs.setPluginEnabled(plugin.id, enabled)
            },
            onInstallClick = { plugin ->
                prefs.setPluginInstalled(plugin.id, true)
                prefs.setPluginEnabled(plugin.id, true)
                refreshPluginsList()
            }
        )

        binding.recyclerViewPlugins.apply {
            layoutManager = LinearLayoutManager(this@MainActivity)
            adapter = pluginAdapter
        }
        binding.btnInstallPluginUrl.setOnClickListener {
            showInstallPluginDialog()
        }
        refreshPluginsList()
    }

    private fun showInstallPluginDialog() {
        val dialogBinding = DialogOpenUrlBinding.inflate(LayoutInflater.from(this))
        dialogBinding.tilUrl.hint = getString(R.string.plugins_url_hint)
        dialogBinding.btnPlayDialog.text = getString(R.string.plugins_install_action)

        val dialog = AlertDialog.Builder(this)
            .setTitle(R.string.plugins_install_url)
            .setView(dialogBinding.root)
            .setCancelable(true)
            .create()

        dialogBinding.btnCancelDialog.setOnClickListener { dialog.dismiss() }
        dialogBinding.btnPlayDialog.setOnClickListener {
            val url = dialogBinding.etUrl.text?.toString()?.trim().orEmpty()
            if (!url.startsWith("http://") && !url.startsWith("https://")) {
                dialogBinding.tilUrl.error = getString(R.string.plugins_url_invalid)
                return@setOnClickListener
            }
            dialogBinding.tilUrl.error = null
            dialog.dismiss()

            lifecycleScope.launch {
                val result = DynamicPluginManager.fetchAndInstall(this@MainActivity, url)
                result.onSuccess { plugin ->
                    prefs.setPluginEnabled(plugin.id, true)
                    Toast.makeText(
                        this@MainActivity,
                        getString(R.string.plugins_install_success, plugin.name),
                        Toast.LENGTH_SHORT
                    ).show()
                    refreshPluginsList()
                }.onFailure { err ->
                    Toast.makeText(
                        this@MainActivity,
                        getString(R.string.plugins_install_failed, err.message ?: "error"),
                        Toast.LENGTH_LONG
                    ).show()
                }
            }
        }
        dialog.show()
    }

    private fun refreshPluginsList() {
        val catalog = MnemisPluginRegistry.allPlugins().map { plugin ->
            PluginItem(
                id = plugin.id,
                name = plugin.name,
                version = plugin.version,
                description = plugin.description,
                isBuiltIn = plugin.isBuiltIn,
                isInstalled = true,
                isEnabled = prefs.isPluginEnabled(plugin.id)
            )
        }

        val filtered = if (currentQuery.isEmpty()) {
            catalog
        } else {
            catalog.filter {
                it.name.contains(currentQuery, ignoreCase = true) ||
                    it.description.contains(currentQuery, ignoreCase = true)
            }
        }
        pluginAdapter.submitList(filtered)
    }

    private fun setupDownloadsSection() {
        binding.recyclerViewDownloads.apply {
            layoutManager = LinearLayoutManager(this@MainActivity)
            adapter = downloadsAdapter
        }
    }

    private fun setupSettingsSection() {
        loadSettingsIntoUi()
        binding.btnSaveSettings.setOnClickListener {
            prefs.defaultUsername = binding.etSettingsUsername.text?.toString().orEmpty()
            prefs.relayServerUrl = binding.etSettingsRelayUrl.text?.toString().orEmpty()
            prefs.adBlockEnabled = binding.switchSettingsAdBlock.isChecked
            prefs.koalaSyncPluginEnabled = binding.switchSettingsKoalaSync.isChecked
            Toast.makeText(this, getString(R.string.settings_saved), Toast.LENGTH_SHORT).show()
        }
        binding.btnCheckUpdates.setOnClickListener {
            checkAppUpdates(manual = true)
        }
    }

    private fun loadSettingsIntoUi() {
        binding.etSettingsUsername.setText(prefs.defaultUsername)
        binding.etSettingsRelayUrl.setText(prefs.relayServerUrl)
        binding.switchSettingsAdBlock.isChecked = prefs.adBlockEnabled
        binding.switchSettingsKoalaSync.isChecked = prefs.koalaSyncPluginEnabled
        binding.tvCurrentVersion.text = getString(R.string.updater_current_version, BuildConfig.VERSION_NAME)
    }

    private fun checkAppUpdates(manual: Boolean) {
        lifecycleScope.launch {
            val result = MnemisUpdater.checkForUpdates(BuildConfig.VERSION_NAME)
            result.onSuccess { update ->
                if (update != null) {
                    showUpdateDialog(update)
                } else if (manual) {
                    Toast.makeText(this@MainActivity, R.string.updater_up_to_date, Toast.LENGTH_SHORT).show()
                }
            }.onFailure {
                if (manual) {
                    Toast.makeText(this@MainActivity, R.string.updater_check_failed, Toast.LENGTH_SHORT).show()
                }
            }
        }
    }

    private fun showUpdateDialog(update: MnemisUpdater.UpdateInfo) {
        if (isFinishing || isDestroyed) return
        val notes = update.releaseNotes.ifBlank { getString(R.string.updater_default_notes) }
        AlertDialog.Builder(this)
            .setTitle(getString(R.string.updater_available_title, update.latestVersion))
            .setMessage(notes)
            .setPositiveButton(R.string.updater_download_action) { _, _ ->
                if (!MnemisUpdater.isApkUrl(update.downloadUrl)) {
                    startActivity(Intent(Intent.ACTION_VIEW, Uri.parse(update.downloadUrl)))
                } else {
                    downloadAndInstallUpdate(update)
                }
            }
            .setNegativeButton(R.string.cancel, null)
            .show()
    }

    private fun downloadAndInstallUpdate(update: MnemisUpdater.UpdateInfo) {
        val safeVersion = update.latestVersion
            .filter { it.isLetterOrDigit() || it == '.' || it == '-' }
            .ifEmpty { "latest" }
        val downloadsDir = getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS)
        if (downloadsDir == null) {
            Toast.makeText(this, R.string.updater_check_failed, Toast.LENGTH_SHORT).show()
            return
        }
        val apkFile = File(downloadsDir, "mnemis-$safeVersion.apk")

        val progressBar = ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal).apply {
            max = 100
            progress = 0
        }
        val progressContainer = FrameLayout(this).apply {
            val padding = (16 * resources.displayMetrics.density).toInt()
            setPadding(padding, padding, padding, padding)
            addView(
                progressBar,
                FrameLayout.LayoutParams(
                    FrameLayout.LayoutParams.MATCH_PARENT,
                    FrameLayout.LayoutParams.WRAP_CONTENT
                )
            )
        }
        val progressDialog = AlertDialog.Builder(this)
            .setTitle(R.string.updater_downloading)
            .setView(progressContainer)
            .setNegativeButton(R.string.cancel) { _, _ ->
                updateDownloadJob?.cancel()
            }
            .setCancelable(false)
            .create()
        progressDialog.show()

        updateDownloadJob = lifecycleScope.launch {
            try {
                val result = MnemisUpdater.downloadApk(update.downloadUrl, apkFile) { downloaded, total ->
                    runOnUiThread {
                        if (total > 0) {
                            progressBar.isIndeterminate = false
                            progressBar.progress =
                                ((downloaded * 100L) / total).toInt().coerceIn(0, 100)
                        } else {
                            progressBar.isIndeterminate = true
                        }
                    }
                }
                if (isFinishing || isDestroyed) return@launch
                result.onSuccess { file ->
                    requestInstallIfNeeded(file)
                }.onFailure {
                    Toast.makeText(this@MainActivity, R.string.updater_check_failed, Toast.LENGTH_LONG).show()
                }
            } finally {
                if (!isFinishing && !isDestroyed && progressDialog.isShowing) {
                    progressDialog.dismiss()
                }
            }
        }
    }

    private fun requestInstallIfNeeded(apkFile: File) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O &&
            !packageManager.canRequestPackageInstalls()
        ) {
            pendingInstallApk = apkFile
            val settingsIntent = Intent(
                Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES,
                Uri.parse("package:$packageName")
            )
            installPermissionLauncher.launch(settingsIntent)
        } else if (hasSignatureConflict(apkFile)) {
            showSignatureConflictDialog()
        } else {
            startApkInstall(apkFile)
        }
    }

    /**
     * Returns true when the APK file is signed with a different key than the
     * Mnemis app currently installed on this device. That happens for APKs
     * installed before the shared debug keystore fix: each CI run used a fresh
     * ephemeral key, so Android refuses to install the new APK over the old
     * one ("package conflicts with an existing package"). When true, the user
     * must uninstall the old app once and then install the new version.
     * Returns false when the signatures match, when no Mnemis is installed,
     * or when the signatures cannot be determined (let the installer decide).
     */
    @Suppress("DEPRECATION")
    private fun hasSignatureConflict(apkFile: File): Boolean {
        return try {
            val installed = packageManager.getPackageInfo(
                packageName,
                PackageManager.GET_SIGNATURES
            )
            val installedSigs = installed.signatures
                ?.map { it.toByteArray().contentHashCode() }
                ?.toSet() ?: return false
            if (installedSigs.isEmpty()) return false
            val archive = packageManager.getPackageArchiveInfo(
                apkFile.absolutePath,
                PackageManager.GET_SIGNATURES
            ) ?: return false
            val apkSigs = archive.signatures
                ?.map { it.toByteArray().contentHashCode() }
                ?.toSet() ?: return false
            if (apkSigs.isEmpty()) return false
            installedSigs != apkSigs
        } catch (_: Exception) {
            false
        }
    }

    private fun showSignatureConflictDialog() {
        if (isFinishing || isDestroyed) return
        AlertDialog.Builder(this)
            .setTitle(R.string.updater_install_conflict_title)
            .setMessage(R.string.updater_install_conflict_message)
            .setPositiveButton(R.string.updater_uninstall_action) { _, _ ->
                startActivity(
                    Intent(Intent.ACTION_DELETE, Uri.parse("package:$packageName"))
                )
            }
            .setNegativeButton(R.string.cancel, null)
            .show()
    }

    private fun startApkInstall(apkFile: File) {
        val uri = FileProvider.getUriForFile(
            this,
            "${BuildConfig.APPLICATION_ID}.fileprovider",
            apkFile
        )
        val installIntent = Intent(Intent.ACTION_VIEW).apply {
            setDataAndType(uri, "application/vnd.android.package-archive")
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION or Intent.FLAG_ACTIVITY_NEW_TASK)
        }
        startActivity(installIntent)
    }

    private fun setupBackNavigation() {
        onBackPressedDispatcher.addCallback(this, object : OnBackPressedCallback(true) {
            override fun handleOnBackPressed() {
                when {
                    binding.drawerLayout.isDrawerOpen(GravityCompat.START) -> {
                        binding.drawerLayout.closeDrawer(GravityCompat.START)
                    }
                    binding.etSearchBar.visibility == View.VISIBLE -> {
                        binding.etSearchBar.setText("")
                        binding.etSearchBar.visibility = View.GONE
                        binding.tvToolbarTitle.visibility = View.VISIBLE
                    }
                    currentSection == Section.VIDEOS && selectedAlbumName != null -> {
                        selectedAlbumName = null
                        applySearchFilter()
                    }
                    currentSection != Section.HOME -> {
                        binding.navigationView.setCheckedItem(R.id.nav_home)
                        switchSection(Section.HOME)
                    }
                    else -> {
                        finish()
                    }
                }
            }
        })
    }

    private fun checkPermissionAndLoadVideos() {
        val permission = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            Manifest.permission.READ_MEDIA_VIDEO
        } else {
            Manifest.permission.READ_EXTERNAL_STORAGE
        }

        if (ContextCompat.checkSelfPermission(this, permission) == PackageManager.PERMISSION_GRANTED) {
            loadLocalVideos()
        } else {
            permissionLauncher.launch(permission)
        }
    }

    private fun requestStoragePermission() {
        val permission = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            Manifest.permission.READ_MEDIA_VIDEO
        } else {
            Manifest.permission.READ_EXTERNAL_STORAGE
        }
        permissionLauncher.launch(permission)
    }

    private fun loadLocalVideos() {
        binding.progressBar.visibility = View.VISIBLE
        binding.layoutEmpty.visibility = View.GONE

        lifecycleScope.launch(Dispatchers.IO) {
            val videos = queryMediaStoreVideos()
            withContext(Dispatchers.Main) {
                binding.progressBar.visibility = View.GONE
                binding.sectionVideos.isRefreshing = false
                allVideos = videos
                applySearchFilter()
            }
        }
    }

    private fun applySearchFilter() {
        val searchFiltered = if (currentQuery.isEmpty()) {
            allVideos
        } else {
            allVideos.filter { it.title.contains(currentQuery, ignoreCase = true) }
        }

        val activeAlbum = selectedAlbumName
        if (activeAlbum == null && currentQuery.isEmpty()) {
            val albums = allVideos
                .groupBy { it.bucketName }
                .map { (name, items) -> VideoAlbum(name = name, coverUri = items.first().uri, videos = items) }
                .sortedByDescending { it.videos.size }

            binding.layoutAlbumHeader.visibility = View.GONE
            binding.recyclerViewAlbums.visibility = if (albums.isEmpty()) View.GONE else View.VISIBLE
            binding.recyclerViewVideos.visibility = View.GONE
            albumAdapter.submitList(albums)

            if (albums.isEmpty()) {
                binding.layoutEmpty.visibility = View.VISIBLE
                binding.tvEmptyMessage.text = getString(R.string.no_videos_found)
                binding.btnGrantPermission.visibility = View.GONE
            } else {
                binding.layoutEmpty.visibility = View.GONE
            }
        } else {
            val visibleVideos = if (activeAlbum != null) {
                searchFiltered.filter { it.bucketName == activeAlbum }
            } else {
                searchFiltered
            }

            binding.layoutAlbumHeader.visibility = if (activeAlbum != null) View.VISIBLE else View.GONE
            binding.tvCurrentAlbumName.text = activeAlbum.orEmpty()
            binding.recyclerViewAlbums.visibility = View.GONE
            binding.recyclerViewVideos.visibility = if (visibleVideos.isEmpty()) View.GONE else View.VISIBLE
            videoAdapter.submitList(visibleVideos)

            if (visibleVideos.isEmpty()) {
                binding.layoutEmpty.visibility = View.VISIBLE
                binding.tvEmptyMessage.text = if (currentQuery.isNotEmpty()) {
                    getString(R.string.search_no_results, currentQuery)
                } else {
                    getString(R.string.no_videos_found)
                }
                binding.btnGrantPermission.visibility = View.GONE
            } else {
                binding.layoutEmpty.visibility = View.GONE
            }
        }

        val downloadedVideos = searchFiltered.filter {
            it.path.contains("/Download", ignoreCase = true)
        }
        downloadsAdapter.submitList(downloadedVideos)
        binding.tvEmptyDownloads.visibility = if (downloadedVideos.isEmpty()) View.VISIBLE else View.GONE

        if (currentSection == Section.PLUGINS) {
            refreshPluginsList()
        }
    }

    private fun queryMediaStoreVideos(): List<VideoItem> {
        val list = mutableListOf<VideoItem>()
        val projection = arrayOf(
            MediaStore.Video.Media._ID,
            MediaStore.Video.Media.TITLE,
            MediaStore.Video.Media.DURATION,
            MediaStore.Video.Media.SIZE,
            MediaStore.Video.Media.WIDTH,
            MediaStore.Video.Media.HEIGHT,
            MediaStore.Video.Media.DATA,
            MediaStore.Video.Media.BUCKET_DISPLAY_NAME
        )

        val sortOrder = "${MediaStore.Video.Media.DATE_ADDED} DESC"

        try {
            contentResolver.query(
                MediaStore.Video.Media.EXTERNAL_CONTENT_URI,
                projection,
                null,
                null,
                sortOrder
            )?.use { cursor ->
                val idColumn = cursor.getColumnIndexOrThrow(MediaStore.Video.Media._ID)
                val titleColumn = cursor.getColumnIndexOrThrow(MediaStore.Video.Media.TITLE)
                val durationColumn = cursor.getColumnIndexOrThrow(MediaStore.Video.Media.DURATION)
                val sizeColumn = cursor.getColumnIndexOrThrow(MediaStore.Video.Media.SIZE)
                val widthColumn = cursor.getColumnIndexOrThrow(MediaStore.Video.Media.WIDTH)
                val heightColumn = cursor.getColumnIndexOrThrow(MediaStore.Video.Media.HEIGHT)
                val dataColumn = cursor.getColumnIndexOrThrow(MediaStore.Video.Media.DATA)
                val bucketColumn = cursor.getColumnIndex(MediaStore.Video.Media.BUCKET_DISPLAY_NAME)

                while (cursor.moveToNext()) {
                    val id = cursor.getLong(idColumn)
                    val title = cursor.getString(titleColumn) ?: "Video"
                    val duration = cursor.getLong(durationColumn)
                    val size = cursor.getLong(sizeColumn)
                    val width = cursor.getInt(widthColumn)
                    val height = cursor.getInt(heightColumn)
                    val path = cursor.getString(dataColumn) ?: ""
                    val bucketName = if (bucketColumn >= 0) {
                        cursor.getString(bucketColumn)?.takeIf { it.isNotBlank() } ?: "Videos"
                    } else {
                        "Videos"
                    }

                    val contentUri = ContentUris.withAppendedId(
                        MediaStore.Video.Media.EXTERNAL_CONTENT_URI,
                        id
                    )

                    val resolution = if (width > 0 && height > 0) "${width}x${height}" else ""

                    list.add(VideoItem(id, contentUri, title, duration, size, resolution, path, bucketName))
                }
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }

        return list
    }

    private fun showPermissionDeniedState() {
        binding.layoutEmpty.visibility = View.VISIBLE
        binding.tvEmptyMessage.text = "Se requiere permiso para leer los videos de tu dispositivo."
        binding.btnGrantPermission.visibility = View.VISIBLE
    }

    private fun showOpenUrlDialog() {
        val dialogBinding = DialogOpenUrlBinding.inflate(LayoutInflater.from(this))
        val dialog = AlertDialog.Builder(this)
            .setView(dialogBinding.root)
            .setCancelable(true)
            .create()

        dialogBinding.btnCancelDialog.setOnClickListener {
            dialog.dismiss()
        }

        dialogBinding.btnPlayDialog.setOnClickListener {
            val url = dialogBinding.etUrl.text?.toString()?.trim()
            if (url.isNullOrEmpty()) {
                dialogBinding.tilUrl.error = "Por favor ingresa una URL válida"
                return@setOnClickListener
            }

            dialogBinding.tilUrl.error = null
            dialog.dismiss()
            openWebPlayer(url, "Web Player")
        }

        dialog.show()
    }

    private fun openPlayer(videoUriString: String, title: String) {
        val intent = Intent(this, PlayerActivity::class.java).apply {
            putExtra(PlayerActivity.EXTRA_VIDEO_URI, videoUriString)
            putExtra(PlayerActivity.EXTRA_VIDEO_TITLE, title)
        }
        startActivity(intent)
    }

    private fun openWebPlayer(webUrl: String, title: String) {
        val intent = Intent(this, WebPlayerActivity::class.java).apply {
            putExtra(WebPlayerActivity.EXTRA_WEB_URL, webUrl)
            putExtra(WebPlayerActivity.EXTRA_WEB_TITLE, title)
        }
        startActivity(intent)
    }

    private fun handleIncomingIntent(intent: Intent?) {
        if (intent == null) return
        val action = intent.action
        val data = intent.data

        if (Intent.ACTION_VIEW == action && data != null) {
            val uriStr = data.toString()
            if (uriStr.startsWith("http://") || uriStr.startsWith("https://")) {
                openWebPlayer(uriStr, "Web Player")
            } else {
                openPlayer(uriStr, "Video")
            }
        }
    }
}
