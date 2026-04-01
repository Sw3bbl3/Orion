#pragma once
#include <JuceHeader.h>

class OrionIcons
{
public:
    // Tabler-inspired icon paths (MIT licensed source set).
    static juce::Path getPathFromSvg(const juce::String& svgPath)
    {
        return juce::Drawable::parseSVGPath(svgPath);
    }


    static juce::Path getPlayIcon()
    {
        return getPathFromSvg("M8 5v14l11-7z");
    }


    static juce::Path getStopIcon()
    {
        return getPathFromSvg("M6 6h12v12H6z");
    }


    static juce::Path getRecordIcon()
    {
        return getPathFromSvg("M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2z");
    }


    static juce::Path getLoopIcon()
    {
        return getPathFromSvg("M12 4V1L8 5l4 4V6c3.31 0 6 2.69 6 6 0 1.01-.25 1.97-.7 2.8l1.46 1.46C19.54 15.03 20 13.57 20 12c0-4.42-3.58-8-8-8zm0 14c-3.31 0-6-2.69-6-6 0-1.01.25-1.97.7-2.8L5.24 7.74C4.46 8.97 4 10.43 4 12c0 4.42 3.58 8 8 8v3l4-4-4-4v3z");
    }

    static juce::Path getRewindIcon()
    {
        return getPathFromSvg("M11 5L3 12l8 7V5zm10 0l-8 7 8 7V5z");
    }

    static juce::Path getFastForwardIcon()
    {
        return getPathFromSvg("M13 5l8 7-8 7V5zM3 5l8 7-8 7V5z");
    }

    static juce::Path getPanicIcon()
    {
        return getPathFromSvg("M12 2L1 21h22L12 2zm-1 7h2v5h-2zm0 7h2v2h-2z");
    }


    static juce::Path getClickIcon()
    {
        return getPathFromSvg("M12 2L2 22h20L12 2zm0 3l7.5 15h-15L12 5zm-1 10h2v5h-2z");
    }


    static juce::Path getSelectIcon()
    {

        return getPathFromSvg("M7 2v11l3-3 2 4 2-1-2-4 4 0z");
    }

    static juce::Path getDrawIcon()
    {

        return getPathFromSvg("M3 17.25V21h3.75L17.81 9.94l-3.75-3.75L3 17.25zM20.71 7.04c.39-.39.39-1.02 0-1.41l-2.34-2.34c-.39-.39-1.02-.39-1.41 0l-1.83 1.83 3.75 3.75 1.83-1.83z");
    }

    static juce::Path getEraseIcon()
    {

        return getPathFromSvg("M15.14 3c-.51 0-1.02.2-1.41.59L2.59 14.73c-.78.78-.78 2.05 0 2.83L5.17 20h13.66v-2h-8.83l9.41-9.41c.78-.78.78-2.05 0-2.83L16.55 3.59c-.39-.39-.9-.59-1.41-.59z");
    }

    static juce::Path getBladeIcon()
    {

        return getPathFromSvg("M9.64 7.64c.23-.5.36-1.05.36-1.64 0-2.21-1.79-4-4-4S2 3.79 2 6s1.79 4 4 4c.59 0 1.14-.13 1.64-.36L10 12l-2.36 2.36C7.14 14.13 6.59 14 6 14c-2.21 0-4 1.79-4 4s1.79 4 4 4 4-1.79 4-4c0-.59-.13-1.14-.36-1.64L12 14l7 7h3v-1L9.64 7.64zM6 8c-1.1 0-2-.9-2-2s.9-2 2-2 2 .9 2 2-.9 2-2 2zm0 12c-1.1 0-2-.9-2-2s.9-2 2-2 2 .9 2 2-.9 2-2 2zm6-7.5c-.28 0-.5-.22-.5-.5s.22-.5.5-.5.5.22.5.5-.22.5-.5.5zM19 3l-6 6 2 2 7-7V3z");
    }

    static juce::Path getGlueIcon()
    {

        return getPathFromSvg("M3 12c0 2.21 1.79 4 4 4h10V4H7c-2.21 0-4 1.79-4 4v4zm16 2v2h-4v-2h4zm-2-8h2v4h-2V6z");
    }

    static juce::Path getZoomIcon()
    {

        return getPathFromSvg("M15.5 14h-.79l-.28-.27C15.41 12.59 16 11.11 16 9.5 16 5.91 13.09 3 9.5 3S3 5.91 3 9.5 5.91 16 9.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14z");
    }

    static juce::Path getMuteToolIcon()
    {

        return getPathFromSvg("M19 3H5c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h14c1.1 0 2-.9 2-2V5c0-1.1-.9-2-2-2zm-2 12l-1.41 1.41L12 13.41l-3.59 3.59L7 15.59l3.59-3.59L7 8.41 8.41 7l3.59 3.59L15.59 7 17 8.41l-3.59 3.59L17 15z");
    }

    static juce::Path getListenIcon()
    {
        return getPathFromSvg("M12 3a5 5 0 0 0-5 5v4.59l-2.3 2.3c-.63.63-.19 1.71.7 1.71H7v3c0 1.1.9 2 2 2h1v-5h2v5h1c1.1 0 2-.9 2-2v-3h1.6c.89 0 1.33-1.08.7-1.71L17 12.59V8a5 5 0 0 0-5-5z");
    }

    static juce::Path getMuteIcon()
    {

        return getPathFromSvg("M16.5 12c0-1.77-1.02-3.29-2.5-4.03v2.21l2.45 2.45c.03-.2.05-.41.05-.63zm2.5 0c0 .94-.2 1.82-.54 2.64l1.51 1.51C20.63 14.91 21 13.5 21 12c0-4.28-2.99-7.86-7-8.77v2.06c2.89.86 5 3.54 5 6.71zM4.27 3L3 4.27 7.73 9H3v6h4l5 5v-6.73l4.25 4.25c-.67.52-1.42.93-2.25 1.18v2.06c1.38-.31 2.63-.95 3.69-1.81L19.73 21 21 19.73 4.27 3zM12 4L9.91 6.09 12 8.18V4z");
    }

    static juce::Path getSoloIcon()
    {

        return getPathFromSvg("M12 3c-4.97 0-9 4.03-9 9v7c0 1.1.9 2 2 2h4v-8H5v-1c0-3.87 3.13-7 7-7s7 3.13 7 7v1h-4v8h4c1.1 0 2-.9 2-2v-7c0-4.97-4.03-9-9-9z");
    }


    static juce::Path getDetachIcon()
    {

        return getPathFromSvg("M19 19H5V5h7V3H5c-1.11 0-2 .9-2 2v14c0 1.1.89 2 2 2h14c1.1 0 2-.9 2-2v-7h-2v7zM14 3v2h3.59l-9.83 9.83 1.41 1.41L19 6.41V10h2V3h-7z");
    }


    static juce::Path getHomeIcon()
    {
        return getPathFromSvg("M10 20v-6h4v6h5v-8h3L12 3 2 12h3v8z");
    }

    static juce::Path getFolderIcon()
    {
        return getPathFromSvg("M10 4H4c-1.1 0-1.99.9-1.99 2L2 18c0 1.1.9 2 2 2h16c1.1 0 2-.9 2-2V8c0-1.1-.9-2-2-2h-8l-2-2z");
    }

    static juce::Path getClockIcon()
    {
        return getPathFromSvg("M11.99 2C6.47 2 2 6.48 2 12s4.47 10 9.99 10C17.52 22 22 17.52 22 12S17.52 2 11.99 2zM12 20c-4.42 0-8-3.58-8-8s3.58-8 8-8 8 3.58 8 8-3.58 8-8 8zm.5-13H11v6l5.25 3.15.75-1.23-4.5-2.67z");
    }

    static juce::Path getStarIcon()
    {
        return getPathFromSvg("M12 17.27L18.18 21l-1.64-7.03L22 9.24l-7.19-.61L12 2 9.19 8.63 2 9.24l5.46 4.73L5.82 21z");
    }

    static juce::Path getSettingsIcon()
    {
        return getPathFromSvg("M19.14 12.94c.04-.3.06-.61.06-.94 0-.32-.02-.64-.07-.94l2.03-1.58c.18-.14.23-.41.12-.61l-1.92-3.32c-.12-.22-.37-.29-.59-.22l-2.39.96c-.5-.38-1.03-.7-1.62-.94l-.36-2.54c-.04-.24-.24-.41-.48-.41h-3.84c-.24 0-.43.17-.47.41l-.36 2.54c-.59.24-1.13.57-1.62.94l-2.39-.96c-.22-.08-.47 0-.59.22L2.74 8.87c-.12.21-.08.47.12.61l2.03 1.58c-.05.3-.09.63-.09.94s.02.64.07.94l-2.03 1.58c-.18.14-.23.41-.12.61l1.92 3.32c.12.22.37.29.59.22l2.39-.96c.5.38 1.03.7 1.62.94l.36 2.54c.05.24.24.41.48.41h3.84c.24 0 .44-.17.47-.41l.36-2.54c.59-.24 1.13-.56 1.62-.94l2.39.96c.22.08.47 0 .59-.22l1.92-3.32c.12-.22.07-.47-.12-.61l-2.01-1.58zM12 15.6c-1.98 0-3.6-1.62-3.6-3.6s1.62-3.6 3.6-3.6 3.6 1.62 3.6 3.6-1.62 3.6-3.6 3.6z");
    }

    static juce::Path getDocumentationIcon()
    {
        return getPathFromSvg("M18 2H6c-1.1 0-2 .9-2 2v16c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V4c0-1.1-.9-2-2-2zM6 4h5v8l-2.5-1.5L6 12V4z");
    }

    static juce::Path getCommunityIcon()
    {
        return getPathFromSvg("M16 11c1.66 0 2.99-1.34 2.99-3S17.66 5 16 5c-1.66 0-3 1.34-3 3s1.34 3 3 3zm-8 0c1.66 0 2.99-1.34 2.99-3S9.66 5 8 5C6.34 5 5 6.34 5 8s1.34 3 3 3zm0 2c-2.33 0-7 1.17-7 3.5V19h14v-2.5c0-2.33-4.67-3.5-7-3.5zm8 0c-.29 0-.62.02-.97.05 1.16.84 1.97 1.97 1.97 3.45V19h6v-2.5c0-2.33-4.67-3.5-7-3.5z");
    }

    static juce::Path getPluginIcon()
    {
        return getPathFromSvg("M4 4h16v16H4V4zm7 4v3H8v2h3v3h2v-3h3v-2h-3V8h-2z");
    }

    static juce::Path getMenuIcon()
    {
        return getPathFromSvg("M3 6h18v2H3V6zm0 5h18v2H3v-2zm0 5h18v2H3v-2z");
    }

    static juce::Path getTrashIcon()
    {
        return getPathFromSvg("M9 3h6l1 2h4v2H4V5h4l1-2zm-2 6h2v9H7V9zm4 0h2v9h-2V9zm4 0h2v9h-2V9z");
    }

    static juce::Path getCloseIcon()
    {
        return getPathFromSvg("M18.3 5.71L12 12l6.3 6.29-1.41 1.41L10.59 13.4 4.29 19.7 2.88 18.3 9.17 12 2.88 5.71 4.29 4.29 10.59 10.6 16.89 4.29z");
    }

    static juce::Path getMinusIcon()
    {
        return getPathFromSvg("M4 11h16v2H4z");
    }

    static juce::Path getMaximizeIcon()
    {
        return getPathFromSvg("M4 4h16v16H4V4zm2 2v12h12V6H6z");
    }

    static juce::Path getPinIcon()
    {
        return getPathFromSvg("M14 3l7 7-2 2-3-3-4 4v7h-2v-7L6 9 3 12 1 10l7-7h6z");
    }

    static juce::Path getGithubIcon()
    {
        return getPathFromSvg("M12 .297c-6.63 0-12 5.373-12 12 0 5.303 3.438 9.8 8.205 11.385.6.113.82-.258.82-.577 0-.285-.01-1.04-.015-2.04-3.338.724-4.042-1.61-4.042-1.61C4.422 18.07 3.633 17.7 3.633 17.7c-1.087-.744.084-.729.084-.729 1.205.084 1.838 1.236 1.838 1.236 1.07 1.835 2.809 1.305 3.495.998.108-.776.417-1.305.76-1.605-2.665-.3-5.466-1.332-5.466-5.93 0-1.31.465-2.38 1.235-3.22-.135-.303-.54-1.523.105-3.176 0 0 1.005-.322 3.3 1.23.96-.267 1.98-.399 3-.405 1.02.006 2.04.138 3 .405 2.28-1.552 3.285-1.23 3.285-1.23.645 1.653.24 2.873.12 3.176.765.84 1.23 1.91 1.23 3.22 0 4.61-2.805 5.625-5.475 5.92.42.36.81 1.096.81 2.22 0 1.606-.015 2.896-.015 3.286 0 .315.21.69.825.57C20.565 22.092 24 17.592 24 12.297c0-6.627-5.373-12-12-12");
    }

    static juce::Path getMixerIcon()
    {
        return getPathFromSvg("M10 20h4V4h-4v16zm-6 0h4v-8H4v8zM16 9v11h4V9h-4z");
    }

    static juce::Path getInspectorIcon()
    {
        return getPathFromSvg("M3 13h2v-2H3v2zm0 4h2v-2H3v2zm0-8h2V7H3v2zm4 4h14v-2H7v2zm0 4h14v-2H7v2zM7 7v2h14V7H7z");
    }

    static juce::Path getPianoRollIcon()
    {
        return getPathFromSvg("M12 3v10.55c-.59-.34-1.27-.55-2-.55-2.21 0-4 1.79-4 4s1.79 4 4 4 4-1.79 4-4V7h4V3h-4z");
    }

    static juce::Path getBrowserIcon()
    {
        return getPathFromSvg("M4 6H2v14c0 1.1.9 2 2 2h14v-2H4V6zm16-4H8c-1.1 0-2 .9-2 2v12c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V4c0-1.1-.9-2-2-2zm-1 9H9V9h10v2zm-4 4H9v-2h6v2zm4-8H9V5h10v2z");
    }

    static juce::Path getImportIcon()
    {
        return getPathFromSvg("M19 9h-4V3H9v6H5l7 7 7-7zM5 18v2h14v-2H5z");
    }

    static juce::Path getExportIcon()
    {
        return getPathFromSvg("M5 9h4V3h6v6h4l-7 7-7-7zM5 18v2h14v-2H5z");
    }

    static juce::Path getPlusIcon()
    {
        return getPathFromSvg("M19 13h-6v6h-2v-6H5v-2h6V5h2v6h6v2z");
    }

    static juce::Path getRefreshIcon()
    {
        return getPathFromSvg("M17.65 6.35C16.2 4.9 14.21 4 12 4c-4.42 0-7.99 3.58-7.99 8s3.57 8 7.99 8c3.73 0 6.84-2.55 7.73-6h-2.08c-.82 2.33-3.04 4-5.65 4-3.31 0-6-2.69-6-6s2.69-6 6-6c1.66 0 3.14.69 4.22 1.78L13 11h7V4l-2.35 2.35z");
    }

    static juce::Path getSaveIcon()
    {
        return getPathFromSvg("M17 3H5c-1.11 0-2 .9-2 2v14c0 1.1.89 2 2 2h14c1.1 0 2-.9 2-2V7l-4-4zm-5 16c-1.66 0-3-1.34-3-3s1.34-3 3-3 3 1.34 3 3-1.34 3-3 3zm3-10H5V5h10v4z");
    }

    static void drawIcon(juce::Graphics& g, const juce::Path& icon, juce::Rectangle<float> bounds, juce::Colour colour)
    {
        g.setColour(colour);

        auto transform = icon.getTransformToScaleToFit(bounds.reduced(4.0f), true);
        g.fillPath(icon, transform);
    }
};
