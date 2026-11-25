#pragma once
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

class SDiveTextDialog : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SDiveTextDialog) {}
        SLATE_ARGUMENT(FText,Title)
        SLATE_ARGUMENT(FText,Initial)
        SLATE_ARGUMENT(bool,English)
        SLATE_ARGUMENT(FText,Hint)
        SLATE_ATTRIBUTE(FText,Error)
        SLATE_EVENT(FOnTextCommitted,OnSubmit)
        SLATE_EVENT(FSimpleDelegate,OnCancel)
    SLATE_END_ARGS()
    TSharedPtr<SEditableTextBox> Editor;
    void Construct(const FArguments& Args)
    {
        Submit=Args._OnSubmit; Cancel=Args._OnCancel;
        ButtonText=FTextBlockStyle().SetFont(FCoreStyle::GetDefaultFontStyle("Regular",18)).SetColorAndOpacity(White);
        ChildSlot
        [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0,.012f,.014f,.88f))
            .HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(40)
            [SNew(SBox).WidthOverride(620)
                [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Panel).Padding(30)
                    [SNew(SVerticalBox)
                        +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,22)
                        [SNew(STextBlock).Text(Args._Title).Font(FCoreStyle::GetDefaultFontStyle("Regular",24)).ColorAndOpacity(White)]
                        +SVerticalBox::Slot().AutoHeight()
                        [SAssignNew(Editor,SEditableTextBox).Text(Args._Initial).Font(FCoreStyle::GetDefaultFontStyle("Regular",22))
                            .SelectAllTextWhenFocused(true).ClearKeyboardFocusOnCommit(false)
                            .OnTextCommitted_Lambda([this](const FText& Text,ETextCommit::Type Type){if(Type==ETextCommit::OnEnter) Submit.ExecuteIfBound(Text,Type);})]
                        +SVerticalBox::Slot().AutoHeight().Padding(0,10,0,0)
                        [SNew(STextBlock).Text(Args._Error).Font(FCoreStyle::GetDefaultFontStyle("Regular",16)).ColorAndOpacity(Amber).WrapTextAt(560)
                            .Visibility_Lambda([Error=Args._Error](){return Error.Get().IsEmpty()?EVisibility::Collapsed:EVisibility::Visible;})]
                        +SVerticalBox::Slot().AutoHeight().Padding(0,12,0,24)
                        [SNew(STextBlock).Text(Args._Hint).Font(FCoreStyle::GetDefaultFontStyle("Regular",16)).ColorAndOpacity(White).WrapTextAt(560)]
                        +SVerticalBox::Slot().AutoHeight()
                        [SNew(SHorizontalBox)
                            +SHorizontalBox::Slot().FillWidth(1).Padding(0,0,10,0)
                            [SNew(SButton).TextStyle(&ButtonText).ContentPadding(FMargin(12,10)).HAlign(HAlign_Center).Text(FText::FromString(Args._English?TEXT("Save"):TEXT("Speichern"))).OnClicked_Lambda([this](){Submit.ExecuteIfBound(Editor->GetText(),ETextCommit::OnEnter); return FReply::Handled();})]
                            +SHorizontalBox::Slot().FillWidth(1)
                            [SNew(SButton).TextStyle(&ButtonText).ContentPadding(FMargin(12,10)).HAlign(HAlign_Center).Text(FText::FromString(Args._English?TEXT("Cancel"):TEXT("Abbrechen"))).OnClicked_Lambda([this](){Cancel.ExecuteIfBound(); return FReply::Handled();})]
                        ]
                    ]
                ]
            ]
        ];
    }
    virtual FReply OnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override
    {
        if(Event.GetKey()==EKeys::Escape) { Cancel.ExecuteIfBound(); return FReply::Handled(); }
        return SCompoundWidget::OnPreviewKeyDown(Geometry,Event);
    }
private:
    const FLinearColor White{.87f,.94f,.92f,1},Amber{1,.73f,.3f,1},Panel{.025f,.065f,.063f,.96f};
    FTextBlockStyle ButtonText;
    FOnTextCommitted Submit;
    FSimpleDelegate Cancel;
};
